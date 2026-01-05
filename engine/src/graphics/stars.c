#include "stars.h"
#include "platform/platform.h"

// Configuration
#define CHUNK_SIZE 128
#define STARS_PER_CHUNK 16
#define NUM_LAYERS 3
#define MAX_VISIBLE_STARS 2048

// Parallax speeds for each layer (0 = farthest, 2 = closest)
static const float layer_speeds_[NUM_LAYERS] = { 0.1f, 0.3f, 0.5f };

// Base brightness for each layer
static const uint8_t layer_min_alpha_[NUM_LAYERS] = { 60, 120, 200 };
static const uint8_t layer_max_alpha_[NUM_LAYERS] = { 100, 180, 255 };

// Star vertex buffer
static float* star_x_;
static float* star_y_;
static uint8_t* star_alpha_;
static size_t star_count_;

// Deterministic hash for chunk coordinates
static inline uint32_t chunk_hash(int32_t cx, int32_t cy, int32_t layer) {
  uint32_t h = (uint32_t)cx * 374761393u 
             + (uint32_t)cy * 668265263u 
             + (uint32_t)layer * 2147483647u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return h ^ (h >> 16);
}

// Fast xorshift for star generation within chunk
static inline uint32_t xorshift32(uint32_t* state) {
  uint32_t x = *state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *state = x;
  return x;
}

void stars_initialize(void) {
  // Allocate star buffers - aligned for SIMD
  star_x_ = (float*)platform_retrieve_memory(MAX_VISIBLE_STARS * sizeof(float));
  star_y_ = (float*)platform_retrieve_memory(MAX_VISIBLE_STARS * sizeof(float));
  star_alpha_ = (uint8_t*)platform_retrieve_memory(MAX_VISIBLE_STARS * sizeof(uint8_t));
}

// Generate stars for a single chunk and add to buffer
static void generate_chunk_stars(int32_t cx, int32_t cy, int32_t layer, 
                                  float offset_x, float offset_y,
                                  uint8_t min_alpha, uint8_t max_alpha) {
  if (star_count_ + STARS_PER_CHUNK > MAX_VISIBLE_STARS)
    return;
    
  uint32_t seed = chunk_hash(cx, cy, layer);
  
  float chunk_world_x = (float)(cx * CHUNK_SIZE);
  float chunk_world_y = (float)(cy * CHUNK_SIZE);
  
  uint8_t alpha_range = max_alpha - min_alpha;
  float norm = (float)CHUNK_SIZE / 65536.0f;
  
  for (int i = 0; i < STARS_PER_CHUNK; i++) {
    // Use lower 16 bits for position (0-65535 -> 0-CHUNK_SIZE)
    uint32_t rx = xorshift32(&seed);
    uint32_t ry = xorshift32(&seed);
    
    float local_x = (float)(rx & 0xFFFF) * norm;
    float local_y = (float)(ry & 0xFFFF) * norm;
    
    star_x_[star_count_] = chunk_world_x + local_x - offset_x;
    star_y_[star_count_] = chunk_world_y + local_y - offset_y;
    
    uint32_t ra = xorshift32(&seed);
    star_alpha_[star_count_] = min_alpha + (uint8_t)(ra % (alpha_range + 1));
    
    star_count_++;
  }
}

void stars_draw(float camera_x, float camera_y) {
  star_count_ = 0;
  
  // Generate stars for each layer
  for (int layer = 0; layer < NUM_LAYERS; layer++) {
    float parallax = layer_speeds_[layer];
    float offset_x = camera_x * parallax;
    float offset_y = camera_y * parallax;
    
    // Calculate visible chunk range for this layer
    // Add margin to prevent popping at edges
    float margin = (float)CHUNK_SIZE;
    
    int min_cx = (int)((offset_x - margin) / CHUNK_SIZE);
    int max_cx = (int)((offset_x + WINDOW_WIDTH + margin) / CHUNK_SIZE);
    int min_cy = (int)((offset_y - margin) / CHUNK_SIZE);
    int max_cy = (int)((offset_y + WINDOW_HEIGHT + margin) / CHUNK_SIZE);
    
    uint8_t min_alpha = layer_min_alpha_[layer];
    uint8_t max_alpha = layer_max_alpha_[layer];
    
    for (int cy = min_cy; cy <= max_cy; cy++) {
      for (int cx = min_cx; cx <= max_cx; cx++) {
        generate_chunk_stars(cx, cy, layer, offset_x, offset_y, min_alpha, max_alpha);
      }
    }
  }
  
  // Render all stars
  if (star_count_ > 0) {
    platform_renderer_draw_stars(star_count_, star_x_, star_y_, star_alpha_);
  }
}


#include "stars.h"
#include "platform/platform.h"
#include "debug/profiler.h"

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

// Star buffers (format for glDrawArrays)
static float* star_vertices_;   // interleaved x,y pairs
static color_t* star_colors_;   // RGBA per star
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
  // Allocate star buffers
  star_vertices_ = (float*)platform_retrieve_memory(MAX_VISIBLE_STARS * 2 * sizeof(float));
  star_colors_ = (color_t*)platform_retrieve_memory(MAX_VISIBLE_STARS * sizeof(color_t));
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

    float screen_x = chunk_world_x + (float)(rx & 0xFFFF) * norm - offset_x;
    float screen_y = chunk_world_y + (float)(ry & 0xFFFF) * norm - offset_y;

    // Simple screen culling
    if (screen_x < 0 || screen_x >= WINDOW_WIDTH || screen_y < 0 || screen_y >= WINDOW_HEIGHT) {
      xorshift32(&seed); // consume alpha random to keep determinism
      continue;
    }

    // Store vertex (interleaved x,y)
    star_vertices_[star_count_ * 2] = screen_x;
    star_vertices_[star_count_ * 2 + 1] = screen_y;

    // Store color (white with varying alpha)
    uint32_t ra = xorshift32(&seed);
    star_colors_[star_count_].r = 255;
    star_colors_[star_count_].g = 255;
    star_colors_[star_count_].b = 255;
    star_colors_[star_count_].a = min_alpha + (uint8_t)(ra % (alpha_range + 1));

    star_count_++;
  }
}

void stars_draw(float camera_x, float camera_y) {
  PROFILE_ZONE("stars_draw");
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

  // Render all stars in one draw call
  if (star_count_ > 0) {
    platform_renderer_draw_stars(star_count_, star_vertices_, star_colors_);
  }
  PROFILE_ZONE_END();
}

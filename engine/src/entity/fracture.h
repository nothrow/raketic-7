#pragma once

#include <stdint.h>
#include "core/core.h"

// Fragment pool configuration
#define FRAGMENT_POOL_SIZE 32
#define FRAGMENT_MAX_VERTICES 48  // 24 line segments max per fragment

// Model indices >= this are dynamic fragments, not static models
#define FRAGMENT_MODEL_BASE 64

// SoA fragment pool - stores dynamically created model fragments
typedef struct {
    __declspec(align(32)) int8_t vertices[FRAGMENT_POOL_SIZE][FRAGMENT_MAX_VERTICES * 2];
    uint8_t vertex_counts[FRAGMENT_POOL_SIZE];
    uint32_t active_mask;  // bitmask for O(1) alloc via _tzcnt_u32
} FragmentPool;

// Result from fracturing a model
typedef struct {
    uint8_t pool_indices[8];   // which fragment pool slots were used
    float centroid_x[8];       // offset from original center
    float centroid_y[8];
    uint8_t count;             // number of fragments created (0-8)
} FractureResult;

// Initialize the fragment pool (called once at startup)
void fragment_pool_initialize(void);

// Allocate a fragment slot from pool
// Returns pool index (0-31), or -1 if pool is full
int fragment_pool_alloc(void);

// Free a fragment slot back to pool
void fragment_pool_free(int pool_idx);

// Draw a fragment (called from _generated_draw_model when index >= FRAGMENT_MODEL_BASE)
void fragment_draw(color_t color, int pool_idx);

// Get pointer to fragment vertices for writing during fracture
int8_t* fragment_get_vertices(int pool_idx);

// Set vertex count for a fragment
void fragment_set_vertex_count(int pool_idx, uint8_t count);

// Fracture a model into fragments
// Returns number of fragments created, fills result with fragment data
int fracture_model(uint16_t model_idx, FractureResult* result);

// Explode an entity: fracture its model and spawn debris particles
// object_idx: index into objects_data array
// Returns number of fragments spawned
int explode_entity(uint32_t object_idx);


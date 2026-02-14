#pragma once

#include <stdint.h>
#include "core/core.h"

// A single polyline on the unit sphere, stored as int16_t (px, py, pz) × 10000
struct surface_polyline {
  const int16_t* verts;  // px, py, pz triplets
  uint16_t count;        // number of vertices
};

// Surface data: a set of polylines + color
struct surface_data {
  const struct surface_polyline* polys;
  uint16_t poly_count;
  color_t color;
};

// Called from generated code: get surface data by index (NULL if invalid)
const struct surface_data* _generated_get_surface(uint16_t index);

// Draw all entity surfaces (called from graphics pipeline)
void surface_draw_all(void);

// Tick surface rotation (called from main tick, before draw)
void surface_tick(void);

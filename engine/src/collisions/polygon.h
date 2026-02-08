#pragma once

#include <stdbool.h>
#include <stdint.h>

#define POLYGON_MAX_VERTICES 64

typedef struct {
  float x[POLYGON_MAX_VERTICES];
  float y[POLYGON_MAX_VERTICES];
  uint32_t count;
} polygon_t;

// Transform int8_t model-space hull vertices to float world-space polygon.
// Hull data is int8_t x,y pairs. Orientation is (ox, oy) unit vector.
void polygon_transform_hull(const int8_t* hull_data, uint32_t vertex_count, float px, float py, float ox, float oy,
                            polygon_t* out);

// SAT (Separating Axis Theorem) test for two convex polygons.
// Returns true if the polygons overlap (collision).
bool polygon_polygon_intersect(const polygon_t* a, const polygon_t* b);

// Ray-segment vs convex polygon intersection test.
// Ray goes from (sx,sy) to (ex,ey). Returns true if the ray hits the polygon.
// On hit, *t_out is set to the parametric distance [0,1] along the ray of the nearest intersection.
bool polygon_ray_intersect(const polygon_t* poly, float sx, float sy, float ex, float ey, float* t_out);

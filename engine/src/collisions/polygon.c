#include "polygon.h"

#include <float.h>
#include <math.h>

void polygon_transform_hull(const int8_t* hull_data, uint32_t vertex_count, float px, float py, float ox, float oy,
                            polygon_t* out) {
  uint32_t count = vertex_count < POLYGON_MAX_VERTICES ? vertex_count : POLYGON_MAX_VERTICES;
  out->count = count;

  for (uint32_t i = 0; i < count; i++) {
    float vx = (float)hull_data[i * 2];
    float vy = (float)hull_data[i * 2 + 1];

    // rotate by orientation (ox, oy) then translate
    out->x[i] = vx * ox - vy * oy + px;
    out->y[i] = vx * oy + vy * ox + py;
  }
}

// Project all vertices of poly onto axis (ax, ay) and return min/max
static void _project(const polygon_t* poly, float ax, float ay, float* out_min, float* out_max) {
  float mn = poly->x[0] * ax + poly->y[0] * ay;
  float mx = mn;

  for (uint32_t i = 1; i < poly->count; i++) {
    float d = poly->x[i] * ax + poly->y[i] * ay;
    if (d < mn)
      mn = d;
    if (d > mx)
      mx = d;
  }

  *out_min = mn;
  *out_max = mx;
}

bool polygon_polygon_intersect(const polygon_t* a, const polygon_t* b) {
  // Test separating axes from polygon A's edges
  for (uint32_t i = 0; i < a->count; i++) {
    uint32_t j = (i + 1) % a->count;

    // Edge vector
    float ex = a->x[j] - a->x[i];
    float ey = a->y[j] - a->y[i];

    // Outward normal (perpendicular)
    float nx = -ey;
    float ny = ex;

    float a_min, a_max, b_min, b_max;
    _project(a, nx, ny, &a_min, &a_max);
    _project(b, nx, ny, &b_min, &b_max);

    if (a_max < b_min || b_max < a_min)
      return false; // separating axis found
  }

  // Test separating axes from polygon B's edges
  for (uint32_t i = 0; i < b->count; i++) {
    uint32_t j = (i + 1) % b->count;

    float ex = b->x[j] - b->x[i];
    float ey = b->y[j] - b->y[i];

    float nx = -ey;
    float ny = ex;

    float a_min, a_max, b_min, b_max;
    _project(a, nx, ny, &a_min, &a_max);
    _project(b, nx, ny, &b_min, &b_max);

    if (a_max < b_min || b_max < a_min)
      return false;
  }

  return true; // no separating axis found -> overlap
}

bool polygon_ray_intersect(const polygon_t* poly, float sx, float sy, float ex, float ey, float* t_out) {
  float dx = ex - sx;
  float dy = ey - sy;

  float best_t = FLT_MAX;
  bool hit = false;

  for (uint32_t i = 0; i < poly->count; i++) {
    uint32_t j = (i + 1) % poly->count;

    float e1x = poly->x[j] - poly->x[i];
    float e1y = poly->y[j] - poly->y[i];

    float denom = dx * e1y - dy * e1x;
    if (fabsf(denom) < 1e-10f)
      continue; // parallel

    float inv_denom = 1.0f / denom;

    float ox = poly->x[i] - sx;
    float oy = poly->y[i] - sy;

    // t = parametric distance along ray [0..1]
    float t = (ox * e1y - oy * e1x) * inv_denom;
    // u = parametric distance along edge [0..1]
    float u = (ox * dy - oy * dx) * inv_denom;

    if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f) {
      if (t < best_t) {
        best_t = t;
        hit = true;
      }
    }
  }

  if (hit)
    *t_out = best_t;

  return hit;
}

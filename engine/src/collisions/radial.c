#include "radial.h"

#include <float.h>
#include <math.h>

// tan(22.5 degrees) = sqrt(2) - 1
#define TAN_22_5 0.41421356f

// Precomputed cos/sin table for 16 sectors (used by reconstruct and ray intersect)
static const float _sector_cos[16] = {
    1.0f, 0.92388f, 0.70711f, 0.38268f, 0.0f, -0.38268f, -0.70711f, -0.92388f,
    -1.0f, -0.92388f, -0.70711f, -0.38268f, 0.0f, 0.38268f, 0.70711f, 0.92388f};
static const float _sector_sin[16] = {
    0.0f, 0.38268f, 0.70711f, 0.92388f, 1.0f, 0.92388f, 0.70711f, 0.38268f,
    0.0f, -0.38268f, -0.70711f, -0.92388f, -1.0f, -0.92388f, -0.70711f, -0.38268f};

int radial_sector(float lx, float ly) {
  // Determine sector [0..15] from direction vector (lx, ly) using only comparisons.
  // Sectors are numbered CCW starting from +X axis, each 22.5 degrees wide.

  float ax = fabsf(lx);
  float ay = fabsf(ly);

  // Octant: which 45-degree slice
  int octant;
  if (ax >= ay) {
    // Closer to X axis. Check half-octant boundary at tan(22.5°)
    octant = (ay > ax * TAN_22_5) ? 1 : 0;
  } else {
    // Closer to Y axis
    octant = (ax > ay * TAN_22_5) ? 2 : 3;
  }

  // Map octant to sector based on quadrant (signs of lx, ly)
  if (lx >= 0) {
    if (ly >= 0) {
      return octant; // Q1: sectors 0-3
    } else {
      // Q4: mirror across X axis
      return (16 - octant) & 15; // sectors 15,14,13,12 for octant 1,2,3,4
    }
  } else {
    if (ly >= 0) {
      // Q2: mirror across Y axis
      return 8 - octant; // sectors 8,7,6,5 for octant 0,1,2,3
    } else {
      // Q3: both negative
      return 8 + octant; // sectors 8,9,10,11
    }
  }
}

bool radial_collision_test(const uint8_t* profile_a, float ax, float ay, float ox_a, float oy_a,
                           const uint8_t* profile_b, float bx, float by, float ox_b, float oy_b) {
  float dx = bx - ax;
  float dy = by - ay;
  float dist_sq = dx * dx + dy * dy;

  // Direction from A to B in A's local frame (complex conjugate multiply)
  float lx_a = dx * ox_a + dy * oy_a;
  float ly_a = dy * ox_a - dx * oy_a;
  int sector_a = radial_sector(lx_a, ly_a);
  int rA = profile_a[sector_a];

  // Direction from B to A in B's local frame (flipped)
  float lx_b = -(dx * ox_b + dy * oy_b);
  float ly_b = -(dy * ox_b - dx * oy_b);
  int sector_b = radial_sector(lx_b, ly_b);
  int rB = profile_b[sector_b];

  int sum = rA + rB;
  return (float)(sum * sum) >= dist_sq;
}

void radial_reconstruct(const uint8_t* profile, float cx, float cy, float ox, float oy,
                        float* out_x, float* out_y) {
  for (int i = 0; i < 16; i++) {
    float r = (float)profile[i];
    // Sector direction in local space
    float local_x = _sector_cos[i] * r;
    float local_y = _sector_sin[i] * r;
    // Rotate by object orientation and translate
    out_x[i] = local_x * ox - local_y * oy + cx;
    out_y[i] = local_x * oy + local_y * ox + cy;
  }
}

bool radial_ray_intersect(const uint8_t* profile, float cx, float cy, float ox, float oy,
                          float sx, float sy, float ex, float ey, float* t_out) {
  // Reconstruct 16-gon from radial profile
  float px[16], py[16];
  radial_reconstruct(profile, cx, cy, ox, oy, px, py);

  float rdx = ex - sx;
  float rdy = ey - sy;

  float best_t = FLT_MAX;
  bool hit = false;

  // Test ray against each of the 16 edges
  for (int i = 0; i < 16; i++) {
    int j = (i + 1) & 15;

    float e1x = px[j] - px[i];
    float e1y = py[j] - py[i];

    float denom = rdx * e1y - rdy * e1x;
    if (fabsf(denom) < 1e-10f)
      continue;

    float inv_denom = 1.0f / denom;

    float ofx = px[i] - sx;
    float ofy = py[i] - sy;

    float t = (ofx * e1y - ofy * e1x) * inv_denom;
    float u = (ofx * rdy - ofy * rdx) * inv_denom;

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

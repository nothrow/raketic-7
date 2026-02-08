#pragma once

#include <stdbool.h>
#include <stdint.h>

#define RADIAL_PROFILE_COUNT 16

// Determine sector index [0..15] from a 2D direction vector.
// Uses only comparisons and one multiply (no trig).
int radial_sector(float lx, float ly);

// Test collision between two objects using radial profiles.
// No trig, no sqrt -- uses complex conjugate multiply for local direction,
// comparison-based sector lookup, squared distance compare.
bool radial_collision_test(const uint8_t* profile_a, float ax, float ay, float ox_a, float oy_a,
                           const uint8_t* profile_b, float bx, float by, float ox_b, float oy_b);

// Ray-segment intersection against an object's radial profile.
// Reconstructs 16 world-space points from profile and tests ray vs 16 edges.
// Returns true on hit, *t_out = parametric [0,1] along ray.
bool radial_ray_intersect(const uint8_t* profile, float cx, float cy, float ox, float oy,
                          float sx, float sy, float ex, float ey, float* t_out);

// Reconstruct 16 world-space points from a radial profile, for debug visualization
// and ray intersection. out_x/out_y must have room for 16 floats.
void radial_reconstruct(const uint8_t* profile, float cx, float cy, float ox, float oy,
                        float* out_x, float* out_y);

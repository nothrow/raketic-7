#include "radial.h"

#include <float.h>
#include <immintrin.h>
#include <math.h>

// tan(22.5 degrees) = sqrt(2) - 1
#define TAN_22_5 0.41421356f

// Precomputed cos/sin table for 16 sectors, 32-byte aligned for AVX loads
__declspec(align(32)) static const float _sector_cos[16] = {
    1.0f, 0.92388f, 0.70711f, 0.38268f, 0.0f, -0.38268f, -0.70711f, -0.92388f,
    -1.0f, -0.92388f, -0.70711f, -0.38268f, 0.0f, 0.38268f, 0.70711f, 0.92388f};
__declspec(align(32)) static const float _sector_sin[16] = {
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
  __m256 vcx = _mm256_set1_ps(cx);
  __m256 vcy = _mm256_set1_ps(cy);
  __m256 vox = _mm256_set1_ps(ox);
  __m256 voy = _mm256_set1_ps(oy);

  for (int batch = 0; batch < 2; batch++) {
    int base = batch * 8;

    // Convert 8 uint8_t radii to float
    __m128i bytes = _mm_loadl_epi64((const __m128i*)(profile + base));
    __m256i ints = _mm256_cvtepu8_epi32(bytes);
    __m256 r = _mm256_cvtepi32_ps(ints);

    // Load precomputed cos/sin for these 8 sectors
    __m256 cos_v = _mm256_load_ps(_sector_cos + base);
    __m256 sin_v = _mm256_load_ps(_sector_sin + base);

    // local_x = cos * r, local_y = sin * r
    __m256 local_x = _mm256_mul_ps(cos_v, r);
    __m256 local_y = _mm256_mul_ps(sin_v, r);

    // Rotate by orientation and translate:
    // out_x = local_x * ox - local_y * oy + cx
    // out_y = local_x * oy + local_y * ox + cy
    __m256 wx = _mm256_fmadd_ps(local_x, vox, vcx);
    wx = _mm256_fnmadd_ps(local_y, voy, wx);

    __m256 wy = _mm256_fmadd_ps(local_x, voy, vcy);
    wy = _mm256_fmadd_ps(local_y, vox, wy);

    _mm256_storeu_ps(out_x + base, wx);
    _mm256_storeu_ps(out_y + base, wy);
  }
}

// Horizontal minimum of 8 floats in an __m256
static inline float _hmin256(__m256 v) {
  __m128 lo = _mm256_castps256_ps128(v);
  __m128 hi = _mm256_extractf128_ps(v, 1);
  __m128 m = _mm_min_ps(lo, hi);
  m = _mm_min_ps(m, _mm_shuffle_ps(m, m, _MM_SHUFFLE(2, 3, 0, 1)));
  m = _mm_min_ps(m, _mm_shuffle_ps(m, m, _MM_SHUFFLE(1, 0, 3, 2)));
  return _mm_cvtss_f32(m);
}

bool radial_ray_intersect(const uint8_t* profile, float cx, float cy, float ox, float oy,
                          float sx, float sy, float ex, float ey, float* t_out) {
  // Reconstruct 16-gon from radial profile, with wrap point for SIMD edge loads
  __declspec(align(32)) float px[17], py[17];
  radial_reconstruct(profile, cx, cy, ox, oy, px, py);
  px[16] = px[0];
  py[16] = py[0];

  float rdx = ex - sx;
  float rdy = ey - sy;

  __m256 vrdx = _mm256_set1_ps(rdx);
  __m256 vrdy = _mm256_set1_ps(rdy);
  __m256 vsx = _mm256_set1_ps(sx);
  __m256 vsy = _mm256_set1_ps(sy);
  __m256 zero = _mm256_setzero_ps();
  __m256 one = _mm256_set1_ps(1.0f);
  __m256 big = _mm256_set1_ps(FLT_MAX);

  __m256 best = big;

  for (int batch = 0; batch < 2; batch++) {
    int base = batch * 8;

    // Current and next polygon points
    __m256 pxi = _mm256_loadu_ps(px + base);
    __m256 pyi = _mm256_loadu_ps(py + base);
    __m256 pxj = _mm256_loadu_ps(px + base + 1);
    __m256 pyj = _mm256_loadu_ps(py + base + 1);

    // Edge vectors: e1 = p[j] - p[i]
    __m256 e1x = _mm256_sub_ps(pxj, pxi);
    __m256 e1y = _mm256_sub_ps(pyj, pyi);

    // denom = rdx * e1y - rdy * e1x
    __m256 denom = _mm256_fmsub_ps(vrdx, e1y, _mm256_mul_ps(vrdy, e1x));

    // inv_denom = 1 / denom (degenerate -> inf, naturally filtered by [0,1] check)
    __m256 inv_denom = _mm256_div_ps(one, denom);

    // Offset from ray start to edge start
    __m256 ofx = _mm256_sub_ps(pxi, vsx);
    __m256 ofy = _mm256_sub_ps(pyi, vsy);

    // t = (ofx * e1y - ofy * e1x) * inv_denom
    __m256 t = _mm256_mul_ps(_mm256_fmsub_ps(ofx, e1y, _mm256_mul_ps(ofy, e1x)), inv_denom);

    // u = (ofx * rdy - ofy * rdx) * inv_denom
    __m256 u = _mm256_mul_ps(_mm256_fmsub_ps(ofx, vrdy, _mm256_mul_ps(ofy, vrdx)), inv_denom);

    // Valid: t in [0,1] AND u in [0,1] (ordered comparisons -> NaN = false)
    __m256 mask = _mm256_and_ps(
        _mm256_and_ps(_mm256_cmp_ps(t, zero, _CMP_GE_OQ), _mm256_cmp_ps(t, one, _CMP_LE_OQ)),
        _mm256_and_ps(_mm256_cmp_ps(u, zero, _CMP_GE_OQ), _mm256_cmp_ps(u, one, _CMP_LE_OQ)));

    // Invalid hits -> FLT_MAX, then take min
    t = _mm256_blendv_ps(big, t, mask);
    best = _mm256_min_ps(best, t);
  }

  float best_t = _hmin256(best);
  if (best_t < FLT_MAX) {
    *t_out = best_t;
    return true;
  }
  return false;
}

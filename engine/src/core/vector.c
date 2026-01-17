#include "core.h"

#include <immintrin.h>
#include <stdlib.h>

float Q_rsqrt(float number) {
  long i;
  float x2, y;
  const float threehalfs = 1.5F;

  x2 = number * 0.5F;
  y = number;
  i = *(long*)&y;            // evil floating point bit level hacking
  i = 0x5f3759df - (i >> 1); // what the fuck?
  y = *(float*)&i;
  y = y * (threehalfs - (x2 * y * y)); // 1st iteration
  //	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

  return y;
}

void vec2_normalize_fast(float* xs, float* ys) {
  float x = xs[0];
  float y = ys[0];
  float len_sq = x * x + y * y;
  float inv_len = Q_rsqrt(len_sq);
  xs[0] = x * inv_len;
  ys[0] = y * inv_len;
}

void vec2_normalize_i(float* xs, float* ys, int count) {
  if (count == 1) {
    vec2_normalize_fast(xs, ys);
    return;
  }

  float* x = xs;
  float* y = ys;
  float* last = x + count;

  __m256 epsilon = _mm256_set1_ps(1.0e-6f);

  for (; x < last; x += 8, y += 8) {

    __m256 vx = _mm256_load_ps(x);
    __m256 vy = _mm256_load_ps(y);

    __m256 vx_sq = _mm256_mul_ps(vx, vx);
    __m256 vy_sq = _mm256_mul_ps(vy, vy);

    __m256 len_sq = _mm256_add_ps(vx_sq, vy_sq);
    __m256 len_sq_safe = _mm256_max_ps(len_sq, epsilon);
    __m256 inv_len = _mm256_rsqrt_ps(len_sq_safe);

    __m256 result = _mm256_mul_ps(vx, inv_len);
    _mm256_store_ps(x, result);

    result = _mm256_mul_ps(vy, inv_len);
    _mm256_store_ps(y, result);
  }
}

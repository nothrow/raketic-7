#include "core.h"

#include <immintrin.h>
#include <stdlib.h>

void vec2_normalize_i(float* xs, float* ys, int count) {
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

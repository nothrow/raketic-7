#include "core.h"

#include <immintrin.h>
#include <stdlib.h>

static uint32_t rng_state = 2463534242u;

uint32_t rand32(void) {
  rng_state ^= rng_state << 13;
  rng_state ^= rng_state >> 17;
  rng_state ^= rng_state << 5;
  return rng_state;
}

vec2_t vec2_random(void) {
  vec2_t v;
  v.x = (float)(rand32() % 1000) / 1000.0f * 2.0f - 1.0f;
  v.y = (float)(rand32() % 1000) / 1000.0f * 2.0f - 1.0f;
  return v;
}

vec2_t vec2_multiply(vec2_t v, float scalar) {
  vec2_t result;
  result.x = v.x * scalar;
  result.y = v.y * scalar;
  return result;
}

vec2_t vec2_add(vec2_t a, vec2_t b) {
  vec2_t result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  return result;
}

void vec2_normalize_i(vec2_t* v, int count) {
  float* start = (float*)v;
  float* last = (float*)&v[count];

  for (; start < last; start += 8) {
    __m256 vec = _mm256_load_ps(start); // x0, y0, x1, y1, x2, y2, x3, y3

    __m256 sq = _mm256_mul_ps(vec, vec); // x0^2, y0^2, x1^2, y1^2, x2^2, y2^2, x3^2, y3^2

    // horizontal add pairs: [len0^2, len1^2, len0^2, len1^2, len2^2, len3^2, len2^2, len3^2]
    __m256 len_sq = _mm256_hadd_ps(sq, sq);

    // shuffle to duplicate: [len0^2, len0^2, len1^2, len1^2, len2^2, len2^2, len3^2, len3^2]
    __m256 len_sq_paired = _mm256_permute_ps(len_sq, 0x50);

    // rsqrt works natively on floats
    __m256 inv_len = _mm256_rsqrt_ps(len_sq_paired);

    __m256 result = _mm256_mul_ps(vec, inv_len);

    _mm256_store_ps(start, result);
  }
}

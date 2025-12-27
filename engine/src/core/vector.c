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
  v.x = (double)(rand32() % 1000) / 1000.0 * 2.0 - 1.0;
  v.y = (double)(rand32() % 1000) / 1000.0 * 2.0 - 1.0;
  return v;
}

vec2_t vec2_multiply(vec2_t v, double scalar) {
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
  double* start = (double*)v;
  double* last = (double*)&v[count];

  for (; start < last; start += 4) {
    __m256d v0 = _mm256_load_pd(start);     // x0, y0, x1, y1
    __m256d v1 = _mm256_load_pd(start + 2); // x2, y2, x3, y3

    __m256d xs = _mm256_unpacklo_pd(v0, v1); // x0, x2, x1, x3
    __m256d ys = _mm256_unpackhi_pd(v0, v1); // y0, y2, y1, y3

    __m256d x2 = _mm256_mul_pd(xs, xs); // x0^2, x2^2, x1^2, x3^2
    __m256d y2 = _mm256_mul_pd(ys, ys); // y0^2, y2^2, y1^2, y3^2

    __m256d sum = _mm256_add_pd(x2, y2); // x0^2 + y0^2, x2^2 + y2^2, x1^2 + y1^2, x3^2 + y3^2

    // we don't have 1/sqrt(x) for doubles, so we convert to float
    __m128 sumf = _mm256_cvtpd_ps(sum);
    __m128 rsqrtf = _mm_rsqrt_ps(sumf);      // approximate 1/sqrt
    __m256d rsqrt = _mm256_cvtps_pd(rsqrtf); // back to double

    __m256d norm_xs = _mm256_mul_pd(xs, rsqrt); // normalized x
    __m256d norm_ys = _mm256_mul_pd(ys, rsqrt); // normalized y

    __m256d res0 = _mm256_unpacklo_pd(norm_xs, norm_ys); // x0, y0, x1, y1
    __m256d res1 = _mm256_unpackhi_pd(norm_xs, norm_ys); // x2, y2, x3, y3

    _mm256_store_pd(start, res0);
    _mm256_store_pd(start + 2, res1);

  }
}

#pragma once

#include <stdint.h>
#include <immintrin.h>

float lut_sin(int32_t deg);
float lut_cos(int32_t deg);

// xorshift RNG
uint32_t rand32(void);

// random float in range [0, 1)
static inline float randf(void) {
  return (float)(rand32() % 10000) / 10000.0f;
}

// random float in range [-1, 1)
static inline float randf_symmetric(void) {
  return randf() * 2.0f - 1.0f;
}

// SIMD: generate 8 random floats in [0, 1)
__m256 randf_8(void);

// SIMD: generate 8 random floats in [-1, 1)
__m256 randf_symmetric_8(void);
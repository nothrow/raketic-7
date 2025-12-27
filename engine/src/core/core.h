#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Debug macros - no standard library dependency in release
#ifdef NDEBUG
#define _ASSERT(x) ((void)0)
#define _VERIFY(x, msg) (x)
#else
#define _ASSERT(x)                                                                                                     \
  do {                                                                                                                 \
    if (!(x))                                                                                                          \
      __debugbreak();                                                                                                  \
  } while (0)
#define _VERIFY(x, msg)                                                                                                \
  do {                                                                                                                 \
    if (!(x))                                                                                                          \
      __debugbreak();                                                                                                  \
  } while (0)
#endif

// Timing constants (120Hz fixed timestep)
#define TICKS_IN_SECOND 120
#define TICK_MS (1000.0 / TICKS_IN_SECOND)
#define TICK_S (1.0 / TICKS_IN_SECOND)

// Core types
typedef struct {
  double x;
  double y;
} vec2_t;

typedef struct {
  uint8_t r, g, b, a;
} color_t;

// Vector operations
vec2_t vec2_random(void);
vec2_t vec2_multiply(vec2_t v, double scalar);
vec2_t vec2_add(vec2_t a, vec2_t b);

void vec2_normalize_i(vec2_t* v, int count);


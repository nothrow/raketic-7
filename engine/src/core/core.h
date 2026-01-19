#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Debug macros - no standard library dependency in release
#ifdef NDEBUG
#define _ASSERT(x)                                                                                                     \
  do {                                                                                                                 \
    __analysis_assume(x);                                                                                              \
    (void)(x);                                                                                                           \
  } while (0);
#define _ASSERTIONS(x) ((void)0)
#else

#include <assert.h>

#define _ASSERTIONS(x)                                                                                                 \
  do {                                                                                                                 \
    x                                                                                                                  \
  } while (0)

#define _ASSERT(x) assert(x)

#endif

// Timing constants (120Hz fixed timestep)
#define TICKS_IN_SECOND 120
#define TICK_MS (1000.0f / TICKS_IN_SECOND)
#define TICK_S (1.0f / TICKS_IN_SECOND)

// for easier passing to renderer
typedef struct {
  float* position_x;
  float* position_y;

  float* orientation_x;
  float* orientation_y;

  float* radius;
} position_orientation_t;

typedef struct {
  uint8_t r, g, b, a;
} color_t;

void vec2_normalize_i(float* xs, float* ys, int count);


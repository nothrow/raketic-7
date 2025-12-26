#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
  double x;
  double y;
} vec2_t;

vec2_t vec2_random(void);
vec2_t vec2_multiply(vec2_t v, double scalar);
vec2_t vec2_add(vec2_t, vec2_t);

void vec2_normalize_i(vec2_t* v, int count);
void vec2_multiply_i(vec2_t* v, double* scalar, int count);


// retrieve memory from fixed heap (or fail)
void memory_initialize(void);
void* retrieve_memory(size_t memory_size);

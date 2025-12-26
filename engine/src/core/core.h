#pragma once

#include <stddef.h>

typedef struct {
  double x;
  double y;
} vec2_t;

// retrieve memory from fixed heap (or fail)
void memory_initialize(void);
void* retrieve_memory(size_t memory_size);

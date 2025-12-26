#pragma once

#include <stddef.h>
#include "core/core.h"

struct objects_data
{
  vec2_t* position;
  vec2_t* velocity;
  vec2_t* acceleration;

  int* model_idx;
  vec2_t* orientation;

  double* mass;
  double* radius;

  size_t active;
  size_t capacity;
};

void entity_manager_initialize(void);

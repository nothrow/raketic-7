#pragma once

#include <stdint.h>
#include "core/core.h"

struct objects_data
{
  uint32_t active;
  uint32_t capacity;

  vec2_t* position;
  vec2_t* velocity;
  double* thrust;

  uint16_t* model_idx;
  vec2_t* orientation;
  
  double* mass;
  double* radius;
};

struct particles_data
{
  uint32_t active;
  uint32_t capacity;

  vec2_t* position;
  vec2_t* velocity;
  
  uint16_t* lifetime_ticks;
  uint16_t* model_idx;

  vec2_t* orientation;
};

void entity_manager_initialize(void);
struct particles_data* entity_manager_get_particles(void);
void entity_manager_pack_particles(void);

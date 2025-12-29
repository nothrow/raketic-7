#pragma once

#include <stdint.h>
#include "core/core.h"

struct objects_data {
  uint32_t active;
  uint32_t capacity;

  float* velocity_x;
  float* velocity_y;
  float* thrust;

  position_orientation_t position_orientation;

  uint16_t* model_idx;

  float* mass;
  float* radius;
};

struct _128bytes {
  uint8_t data[128];
};

struct particles_data {
  uint32_t active;
  uint32_t capacity;

  float* velocity_x;
  float* velocity_y;

  uint16_t* lifetime_ticks;
  uint16_t* lifetime_max;
  uint16_t* model_idx;

  position_orientation_t position_orientation;

  struct _128bytes* temporary; // reserved memory for intermediate computations
};

void entity_manager_initialize(void);
struct particles_data* entity_manager_get_particles(void);
struct objects_data* entity_manager_get_objects(void);
void entity_manager_pack_particles(void);

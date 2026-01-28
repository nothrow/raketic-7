#pragma once

#include "entity_internal.h"

struct engine_data {
  float thrust;
  float power;
  uint16_t particle_model;
};

void engine_part_entity_initialize(void);

#pragma once

#include "entity_internal.h"


typedef struct {
  uint16_t message;

  float x;
  float y;

  int8_t vx;
  int8_t vy;

  uint16_t ttl;
  uint16_t model_idx;
} particle_create_message_t;


void particles_entity_initialize(void);

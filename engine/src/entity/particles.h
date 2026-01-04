#pragma once

#include "entity_internal.h"


typedef struct {
  float x;
  float y;

  float vx;
  float vy;

  float ox;
  float oy;

  uint16_t ttl;
  uint16_t model_idx;
} particle_create_t;


void particles_entity_initialize(void);
void particles_create_particle(particle_create_t* pc);

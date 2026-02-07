#pragma once

#include "entity_internal.h"
#include "graphics/graphics.h"

#define MAX_BEAMS 256

struct beams_data {
  uint32_t active;
  uint32_t capacity;
  
  float* start_x;
  float* start_y;
  float* end_x;
  float* end_y;
  uint16_t* lifetime_ticks;
  uint16_t* lifetime_max;
};

void beams_entity_initialize(void);
void beams_create(float start_x, float start_y, float end_x, float end_y, uint16_t lifetime);
struct beams_data* beams_get_data(void);

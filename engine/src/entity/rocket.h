#pragma once

#include "entity_internal.h"

void rocket_entity_initialize(void);
void rocket_create(float x, float y, float vx, float vy, float ox, float oy,
                   uint16_t model_idx, uint16_t smoke_model_idx,
                   float thrust_power, uint16_t fuel_ticks, uint16_t lifetime_ticks);
void rocket_remap_objects(uint32_t* remap, uint32_t old_active);

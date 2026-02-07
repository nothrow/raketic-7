#pragma once

#include "entity_internal.h"

void rocket_entity_initialize(void);
void rocket_create(float x, float y, float vx, float vy, float ox, float oy,
                   uint16_t model_idx, uint16_t smoke_model_idx);

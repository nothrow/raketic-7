#pragma once

#include "entity_internal.h"

void camera_entity_initialize(void);
void camera_set_entity(entity_id_t entity_id);
void camera_get_absolute_position(double* out_x, double* out_y);
void camera_remap_entity(uint32_t* remap, uint32_t old_active);



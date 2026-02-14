#pragma once

#include "entity_internal.h"

void camera_entity_initialize(void);
void camera_set_entity(entity_id_t entity_id);
void camera_get_absolute_position(double* out_x, double* out_y);
void camera_remap_entity(uint32_t* remap, uint32_t old_active);

// Zoom control (1.0 = default, >1 = zoomed out, <1 = zoomed in)
float camera_get_zoom(void);
void camera_zoom_by(float factor);  // multiplicative: zoom *= factor



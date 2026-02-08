#pragma once

#include "entity/types.h"

void debug_initialize(void);
void debug_watch_set(entity_id_t id);
void debug_watch_draw(void);
void debug_draw_collision_hulls(void);
void debug_draw_orbit_zones(void);
void debug_trails_draw(void);

void debug_toggle_overlay(void);
bool debug_is_overlay_enabled(void);


#pragma once

#include "entity.h"

void autopilot_initialize(void);

// Engage orbit autopilot for a ship around a planet (identified by ordinal in objects_data).
// Can be called by player detection logic or by AI ships.
void autopilot_orbit_engage(entity_id_t ship_id, uint32_t planet_ordinal);

// Disengage orbit autopilot for a ship. Sends ENGINES_THRUST 0 and broadcast DISENGAGE.
void autopilot_orbit_disengage(entity_id_t ship_id);

// Check if a ship has an active orbit autopilot.
bool autopilot_orbit_is_active(entity_id_t ship_id);

// Run one autopilot tick for all active slots. Call from 120HZ_BEFORE_PHYSICS.
void autopilot_tick(void);

// Update stored ordinals after entity_manager_pack_objects.
void autopilot_remap(uint32_t* remap, uint32_t old_active);

// Autopilot status snapshot for HUD display.
typedef struct {
  bool active;
  int phase;      // 0 = coast, 1 = correcting
  float delta_v;  // current delta-v magnitude to ideal orbit
  float v_orbit;  // target circular orbit speed at current distance
} autopilot_status_t;

// Query the current autopilot state for a ship. Safe to call every frame.
void autopilot_query_status(entity_id_t ship_id, autopilot_status_t* out);

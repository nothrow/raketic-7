#pragma once

#include "entity.h"

void ai_initialize(void);

// Engage orbit AI for an entity around a gravitational body (identified by ordinal in objects_data).
// Works with any entity type (ship, satellite, etc.).
void ai_orbit_engage(entity_id_t entity_id, uint32_t body_ordinal);

// Disengage orbit AI for an entity. Sends ENGINES_THRUST 0 and broadcast DISENGAGE.
void ai_orbit_disengage(entity_id_t entity_id);

// Check if an entity has an active orbit AI.
bool ai_orbit_is_active(entity_id_t entity_id);

// Run one AI tick for all active slots. Call from 120HZ_BEFORE_PHYSICS.
void ai_tick(void);

// Update stored ordinals after entity_manager_pack_objects.
void ai_remap(uint32_t* remap, uint32_t old_active);

// AI status snapshot for HUD display.
typedef struct {
  bool active;
  int phase;      // 0 = coast, 1 = correcting
  float delta_v;  // current delta-v magnitude to ideal orbit
  float v_orbit;  // target circular orbit speed at current distance
} ai_status_t;

// Query the current AI state for an entity. Safe to call every frame.
void ai_query_status(entity_id_t entity_id, ai_status_t* out);

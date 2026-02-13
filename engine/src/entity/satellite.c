#include "platform/platform.h"
#include "platform/math.h"

#include "entity_internal.h"
#include "entity.h"
#include "satellite.h"
#include "ai.h"
#include "radar.h"
#include "weapon.h"
#include "explosion.h"

#include <math.h>

// Satellite: orbit-keeping via AI, turret defense driven by radar contacts.
// The satellite does NOT scan for targets itself. It reacts to
// MESSAGE_RADAR_SCAN_COMPLETE from its radar part, reads the contact list,
// filters for hostiles (asteroids), and distributes targets among turrets.

static void _satellite_rotate_to(entity_id_t id, float x, float y) {
  uint32_t obj_idx = GET_ORDINAL(id);
  struct objects_data* od = entity_manager_get_objects();

  od->position_orientation.orientation_x[obj_idx] = x;
  od->position_orientation.orientation_y[obj_idx] = y;

  vec2_normalize_i(&od->position_orientation.orientation_x[obj_idx],
                   &od->position_orientation.orientation_y[obj_idx], 1);
}

// Accumulate engine thrust for satellites (same pattern as ship)
static void _satellite_apply_thrust_from_engines(void) {
  struct objects_data* od = entity_manager_get_objects();
  struct parts_data* pd = entity_manager_get_parts();

  for (size_t i = 0; i < od->active; i++) {
    if (od->type[i]._ != ENTITY_TYPE_SATELLITE) continue;

    od->thrust[i] = 0.0f;

    for (size_t j = 0; j < od->parts_count[i]; j++) {
      uint32_t part_idx = od->parts_start_idx[i] + (uint32_t)j;
      if (pd->type[part_idx]._ != ENTITY_TYPE_PART_ENGINE) continue;

      struct engine_data {
        float thrust;
        float power;
        uint16_t particle_model;
      };
      struct engine_data* ed = (struct engine_data*)(pd->data[part_idx].data);
      od->thrust[i] += ed->thrust;
    }
  }
}

// Returns true if this entity type is a hostile target the satellite should engage.
static bool _is_hostile(uint8_t entity_type) {
  return entity_type == ENTITY_TYPE_ASTEROID;
}

// Radar reported scan results — read contacts, distribute hostile targets among turrets.
static void _satellite_handle_scan_complete(entity_id_t id, int32_t contact_count) {
  if (contact_count < 0) contact_count = 0;

  struct objects_data* od = entity_manager_get_objects();
  struct parts_data* pd = entity_manager_get_parts();
  uint32_t sat_idx = GET_ORDINAL(id);

  if (sat_idx >= od->active) return;

  uint32_t parts_start = od->parts_start_idx[sat_idx];
  uint32_t parts_count = od->parts_count[sat_idx];

  // Find radar part and read contacts
  const struct radar_contact* contacts = NULL;
  uint8_t num_contacts = 0;

  for (uint32_t p = 0; p < parts_count; p++) {
    uint32_t pi = parts_start + p;
    if (pd->type[pi]._ == ENTITY_TYPE_PART_RADAR) {
      struct radar_data* rd = (struct radar_data*)(pd->data[pi].data);
      contacts = rd->contacts;
      num_contacts = rd->contact_count;
      break;
    }
  }

  if (contacts == NULL) return;

  // Filter hostiles from contacts (already sorted by distance, closest first)
  float hostile_x[MAX_RADAR_CONTACTS];
  float hostile_y[MAX_RADAR_CONTACTS];
  uint8_t hostile_count = 0;

  for (uint8_t c = 0; c < num_contacts && hostile_count < MAX_RADAR_CONTACTS; c++) {
    if (_is_hostile(contacts[c].entity_type)) {
      hostile_x[hostile_count] = contacts[c].x;
      hostile_y[hostile_count] = contacts[c].y;
      hostile_count++;
    }
  }

  // Distribute targets among turret laser parts.
  // Each turret gets its own target (round-robin from the hostile list).
  // If more turrets than targets, remaining turrets share the closest target.
  // If no hostiles, all turrets cease fire.
  uint8_t target_idx = 0;

  for (uint32_t p = 0; p < parts_count; p++) {
    uint32_t pi = parts_start + p;
    if (pd->type[pi]._ != ENTITY_TYPE_PART_WEAPON) continue;

    struct weapon_data* wd = (struct weapon_data*)(pd->data[pi].data);
    if (wd->weapon_type != WEAPON_TYPE_TURRET_LASER) continue;

    entity_id_t turret_id = PART_ID_WITH_TYPE(pi, ENTITY_TYPE_PART_WEAPON);

    if (hostile_count > 0) {
      // Assign target: distribute round-robin, wrapping to closest if fewer targets than turrets
      uint8_t tidx = (target_idx < hostile_count) ? target_idx : 0;
      messaging_send(turret_id,
                     CREATE_MESSAGE_F(MESSAGE_WEAPON_SET_TARGET, hostile_x[tidx], hostile_y[tidx]));
      messaging_send(turret_id,
                     CREATE_MESSAGE(MESSAGE_WEAPON_FIRE, 1, 0));
      target_idx++;
    } else {
      messaging_send(turret_id,
                     CREATE_MESSAGE(MESSAGE_WEAPON_FIRE, 0, 0));
    }
  }
}

static void _satellite_handle_collision(entity_id_t id, const message_t* msg) {
  entity_id_t id_a = { (uint32_t)msg->data_a };
  entity_id_t id_b = { (uint32_t)msg->data_b };

  entity_id_t other = (GET_TYPE(id_a) == ENTITY_TYPEREF_SATELLITE._) ? id_b : id_a;
  uint8_t other_type = GET_TYPE(other);

  if (other_type == ENTITY_TYPEREF_PLANET._ || other_type == ENTITY_TYPEREF_MOON._ || other_type == ENTITY_TYPEREF_ASTEROID._) {
    uint32_t ord = GET_ORDINAL(id);
    struct objects_data* od = entity_manager_get_objects();

    if (od->health[ord] <= 0) return;

    float px = od->position_orientation.position_x[ord];
    float py = od->position_orientation.position_y[ord];
    float vx = od->velocity_x[ord];
    float vy = od->velocity_y[ord];
    float mass = od->mass[ord];

    od->health[ord] = 0;
    explosion_spawn(px, py, vx, vy, mass);
  }
}

static void _satellite_handle_beam_hit(entity_id_t id, const message_t* msg) {
  uint32_t ord = GET_ORDINAL(id);
  struct objects_data* od = entity_manager_get_objects();

  if (od->health[ord] <= 0) return;

  int16_t damage = (int16_t)msg->data_b;
  od->health[ord] -= damage;

  if (od->health[ord] <= 0) {
    float px = od->position_orientation.position_x[ord];
    float py = od->position_orientation.position_y[ord];
    float vx = od->velocity_x[ord];
    float vy = od->velocity_y[ord];
    float mass = od->mass[ord];

    explosion_spawn(px, py, vx, vy, mass);
  }
}

static void _satellite_dispatch(entity_id_t id, message_t msg) {
  switch (msg.message) {
  case MESSAGE_ROTATE_TO:
    _satellite_rotate_to(id, _i2f(msg.data_a), _i2f(msg.data_b));
    break;

  case MESSAGE_BROADCAST_120HZ_BEFORE_PHYSICS:
    _satellite_apply_thrust_from_engines();
    break;

  case MESSAGE_RADAR_SCAN_COMPLETE:
    _satellite_handle_scan_complete(id, msg.data_a);
    break;

  case MESSAGE_COLLIDE_OBJECT_OBJECT:
    _satellite_handle_collision(id, &msg);
    break;

  case MESSAGE_BEAM_HIT:
    _satellite_handle_beam_hit(id, &msg);
    break;

  case MESSAGE_AI_ORBIT_DISENGAGE:
    ai_orbit_disengage(id);
    break;
  }
}

void satellite_entity_initialize(void) {
  entity_manager_vtables[ENTITY_TYPE_SATELLITE].dispatch_message = _satellite_dispatch;
}

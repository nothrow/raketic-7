#include "platform/platform.h"
#include "platform/math.h"

#include "entity_internal.h"
#include "entity.h"
#include "satellite.h"
#include "ai.h"
#include "explosion.h"

#include <math.h>

// Turret defense: scan for approaching asteroids and fire turret lasers.
// Evaluation rate and engagement parameters.
#define SAT_SCAN_INTERVAL     6    // re-scan every 6 ticks (~0.05s)
#define SAT_DEFAULT_RANGE   300.0f // default turret engagement range

static void _satellite_rotate_to(entity_id_t id, float x, float y) {
  uint32_t obj_idx = GET_ORDINAL(id);
  struct objects_data* od = entity_manager_get_objects();

  od->position_orientation.orientation_x[obj_idx] = x;
  od->position_orientation.orientation_y[obj_idx] = y;

  vec2_normalize_i(&od->position_orientation.orientation_x[obj_idx],
                   &od->position_orientation.orientation_y[obj_idx], 1);
}

// Find the closest approaching asteroid within range of the satellite.
// Returns true if a target was found, writing its world position into out_x/out_y.
static bool _satellite_find_target(uint32_t sat_idx, float range,
                                   float* out_x, float* out_y) {
  struct objects_data* od = entity_manager_get_objects();

  float sx = od->position_orientation.position_x[sat_idx];
  float sy = od->position_orientation.position_y[sat_idx];

  float best_dist_sq = range * range;
  bool found = false;

  for (uint32_t i = 0; i < od->active; i++) {
    if (od->type[i]._ != ENTITY_TYPE_ASTEROID) continue;
    if (od->health[i] <= 0) continue;

    float dx = od->position_orientation.position_x[i] - sx;
    float dy = od->position_orientation.position_y[i] - sy;
    float dist_sq = dx * dx + dy * dy;

    if (dist_sq < best_dist_sq) {
      best_dist_sq = dist_sq;
      *out_x = od->position_orientation.position_x[i];
      *out_y = od->position_orientation.position_y[i];
      found = true;
    }
  }

  return found;
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

// Per-tick: scan for targets, command turret weapons
static void _satellite_defense_tick(void) {
  struct objects_data* od = entity_manager_get_objects();

  for (uint32_t i = 0; i < od->active; i++) {
    if (od->type[i]._ != ENTITY_TYPE_SATELLITE) continue;

    entity_id_t sat_id = OBJECT_ID_WITH_TYPE(i, ENTITY_TYPE_SATELLITE);

    float target_x, target_y;
    if (_satellite_find_target(i, SAT_DEFAULT_RANGE, &target_x, &target_y)) {
      // Set target and fire
      messaging_send(PARTS_OF_TYPE(sat_id, PART_TYPEREF_WEAPON),
                     CREATE_MESSAGE_F(MESSAGE_WEAPON_SET_TARGET, target_x, target_y));
      messaging_send(PARTS_OF_TYPE(sat_id, PART_TYPEREF_WEAPON),
                     CREATE_MESSAGE(MESSAGE_WEAPON_FIRE, 1, 0));
    } else {
      // No target — cease fire
      messaging_send(PARTS_OF_TYPE(sat_id, PART_TYPEREF_WEAPON),
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
    _satellite_defense_tick();
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

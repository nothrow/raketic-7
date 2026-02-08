
#include "platform/platform.h"
#include "platform/math.h"

#include "entity_internal.h"
#include "entity.h"
#include "engine.h"
#include "ship.h"
#include "autopilot.h"

#include <math.h>

#define MAXSIZE 8192

// === Orbit detection (player intent) ===
// Detects when a ship is near a planet with engines off and tangential trajectory.
// After a delay, engages the autopilot module.

#define GRAVITATIONAL_CONSTANT 6.67430f

#define ORBIT_ENGAGE_DELAY_TICKS 18     // 0.15s of stable conditions before engaging
#define ORBIT_MIN_DISTANCE_FACTOR 1.5f
#define ORBIT_MAX_DISTANCE_FACTOR 3.0f
#define ORBIT_TANGENTIAL_THRESHOLD 0.6f

struct orbit_detect {
  uint32_t ship_ordinal;
  uint32_t planet_ordinal;
  int counter;
};

static struct orbit_detect _detect = {
  .ship_ordinal = UINT32_MAX,
  .planet_ordinal = UINT32_MAX,
  .counter = 0
};

static void _detect_reset(void) {
  _detect.counter = 0;
  _detect.ship_ordinal = UINT32_MAX;
  _detect.planet_ordinal = UINT32_MAX;
}

static void _ship_check_orbit_conditions(struct objects_data* od, uint32_t ship_idx) {
  float sx = od->position_orientation.position_x[ship_idx];
  float sy = od->position_orientation.position_y[ship_idx];
  float svx = od->velocity_x[ship_idx];
  float svy = od->velocity_y[ship_idx];

  float speed_sq = svx * svx + svy * svy;
  if (speed_sq < 0.001f) return;

  float speed = sqrtf(speed_sq);

  // Find nearest planet
  float best_dist_sq = 1e30f;
  uint32_t best_planet = UINT32_MAX;

  for (uint32_t i = 0; i < od->active; i++) {
    if (od->type[i]._ != ENTITY_TYPE_PLANET) continue;

    float dx = od->position_orientation.position_x[i] - sx;
    float dy = od->position_orientation.position_y[i] - sy;
    float dist_sq = dx * dx + dy * dy;

    if (dist_sq < best_dist_sq) {
      best_dist_sq = dist_sq;
      best_planet = i;
    }
  }

  if (best_planet == UINT32_MAX) return;

  float dist = sqrtf(best_dist_sq);
  float planet_radius = od->position_orientation.radius[best_planet];

  if (dist < planet_radius * ORBIT_MIN_DISTANCE_FACTOR) return;
  if (dist > planet_radius * ORBIT_MAX_DISTANCE_FACTOR) return;

  float planet_mass = od->mass[best_planet];
  float v_escape_sq = 2.0f * GRAVITATIONAL_CONSTANT * planet_mass / dist;
  if (speed_sq > v_escape_sq) return;

  float rnx = (sx - od->position_orientation.position_x[best_planet]) / dist;
  float rny = (sy - od->position_orientation.position_y[best_planet]) / dist;
  float v_radial = svx * rnx + svy * rny;

  if (fabsf(v_radial) / speed > ORBIT_TANGENTIAL_THRESHOLD) return;

  // All conditions met — track this pair
  if (_detect.ship_ordinal == ship_idx && _detect.planet_ordinal == best_planet) {
    _detect.counter++;
  } else {
    _detect.ship_ordinal = ship_idx;
    _detect.planet_ordinal = best_planet;
    _detect.counter = 1;
  }

  if (_detect.counter >= ORBIT_ENGAGE_DELAY_TICKS) {
    entity_id_t ship_id = OBJECT_ID_WITH_TYPE(ship_idx, ENTITY_TYPE_SHIP);
    autopilot_orbit_engage(ship_id, best_planet);
    _detect_reset();
  }
}

// === Ship core logic ===

static void _ship_rotate_by(entity_id_t idx, int32_t rotation) {
  uint32_t id = GET_ORDINAL(idx);
  struct objects_data* od = entity_manager_get_objects();

  float ox = od->position_orientation.orientation_x[id];
  float oy = od->position_orientation.orientation_y[id];

  float c = lut_cos(rotation);
  float s = lut_sin(rotation);

  od->position_orientation.orientation_x[id] = ox * c - oy * s;
  od->position_orientation.orientation_y[id] = ox * s + oy * c;

  vec2_normalize_i(&od->position_orientation.orientation_x[id], &od->position_orientation.orientation_y[id], 1);
}

static void _ship_apply_thrust_from_engines(void) {
  struct objects_data* od = entity_manager_get_objects();
  struct parts_data* pd = entity_manager_get_parts();

  for (size_t i = 0; i < od->active; i++) {
    od->thrust[i] = 0.0f;

    for (size_t j = 0; j < od->parts_count[i]; j++) {
      uint32_t part_idx = od->parts_start_idx[i] + j;
      if (pd->type[part_idx]._ != ENTITY_TYPE_PART_ENGINE) {
        continue;
      }
      struct engine_data* ed = (struct engine_data*)(pd->data[part_idx].data);
      od->thrust[i] += ed->thrust;
    }  
  }

  // Orbit detection: only for ships not already in autopilot, with engines off
  for (size_t i = 0; i < od->active; i++) {
    if (od->type[i]._ != ENTITY_TYPE_SHIP) continue;

    entity_id_t ship_id = OBJECT_ID_WITH_TYPE((uint32_t)i, ENTITY_TYPE_SHIP);

    if (autopilot_orbit_is_active(ship_id)) continue;

    if (od->thrust[i] > 0.0f) {
      // Player is thrusting — reset detection if tracking this ship
      if (_detect.ship_ordinal == (uint32_t)i) {
        _detect_reset();
      }
    } else {
      _ship_check_orbit_conditions(od, (uint32_t)i);
    }
  }

  // Tick autopilot for all active slots
  autopilot_tick();
}

static void _ship_rotate_to(entity_id_t id, float x, float y) {
  uint32_t obj_idx = GET_ORDINAL(id);
  struct objects_data* od = entity_manager_get_objects();

  od->position_orientation.orientation_x[obj_idx] = x;
  od->position_orientation.orientation_y[obj_idx] = y;

  vec2_normalize_i(&od->position_orientation.orientation_x[obj_idx], &od->position_orientation.orientation_y[obj_idx], 1);
}

static void _ship_handle_collision(entity_id_t id, const message_t* msg) {
  (id);

  entity_id_t other = { msg->data_b };
  if (GET_TYPE(other) == ENTITY_TYPEREF_PLANET._) {
    // destroy
  }
}

static void _ship_dispatch(entity_id_t id, message_t msg) {
  switch (msg.message) {
  case MESSAGE_SHIP_ROTATE_BY:
    _ship_rotate_by(id, msg.data_a);
    break;
  case MESSAGE_BROADCAST_120HZ_BEFORE_PHYSICS:
    _ship_apply_thrust_from_engines();
    break;
  case MESSAGE_COLLIDE_OBJECT_OBJECT:
    _ship_handle_collision(id, &msg);
    break;
  case MESSAGE_SHIP_ROTATE_TO:
    _ship_rotate_to(id, _i2f(msg.data_a), _i2f(msg.data_b));
    break;
  case MESSAGE_SHIP_AUTOPILOT_DISENGAGE:
    autopilot_orbit_disengage(id);
    break;
  }
}

void ship_entity_initialize(void) {
  entity_manager_vtables[ENTITY_TYPE_SHIP].dispatch_message = _ship_dispatch;
}

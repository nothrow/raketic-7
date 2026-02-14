
#include "platform/platform.h"
#include "platform/math.h"

#include "entity_internal.h"
#include "entity.h"
#include "engine.h"
#include "ship.h"
#include "ai.h"
#include "radar.h"
#include "explosion.h"

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
  entity_id_t ship_id = OBJECT_ID_WITH_TYPE(ship_idx, ENTITY_TYPE_SHIP);

  // Use radar to find the nearest celestial body
  const struct radar_data* rd = radar_get_data(ship_id);
  if (!rd || rd->nav_count == 0) return;

  uint32_t best_planet = (uint32_t)rd->nav[0].entity_ordinal;
  if (best_planet >= od->active) return;

  float sx = od->position_orientation.position_x[ship_idx];
  float sy = od->position_orientation.position_y[ship_idx];
  float bx = od->position_orientation.position_x[best_planet];
  float by = od->position_orientation.position_y[best_planet];

  float dx = bx - sx;
  float dy = by - sy;
  float dist_sq = dx * dx + dy * dy;
  float dist = sqrtf(dist_sq);

  float planet_radius = od->position_orientation.radius[best_planet];

  if (dist < planet_radius * ORBIT_MIN_DISTANCE_FACTOR) return;
  if (dist > planet_radius * ORBIT_MAX_DISTANCE_FACTOR) return;

  // Use velocity RELATIVE to the body (the body itself moves, e.g. planet orbiting sun)
  float rel_vx = od->velocity_x[ship_idx] - od->velocity_x[best_planet];
  float rel_vy = od->velocity_y[ship_idx] - od->velocity_y[best_planet];
  float speed_sq = rel_vx * rel_vx + rel_vy * rel_vy;
  if (speed_sq < 0.001f) return;
  float speed = sqrtf(speed_sq);

  float planet_mass = od->mass[best_planet];
  float v_escape_sq = 2.0f * GRAVITATIONAL_CONSTANT * planet_mass / dist;
  if (speed_sq > v_escape_sq) return;

  float rnx = (sx - bx) / dist;
  float rny = (sy - by) / dist;
  float v_radial = rel_vx * rnx + rel_vy * rny;

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
    ai_orbit_engage(ship_id, best_planet);
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

    if (ai_orbit_is_active(ship_id)) continue;

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
  ai_tick();
}

static void _ship_rotate_to(entity_id_t id, float x, float y) {
  uint32_t obj_idx = GET_ORDINAL(id);
  struct objects_data* od = entity_manager_get_objects();

  od->position_orientation.orientation_x[obj_idx] = x;
  od->position_orientation.orientation_y[obj_idx] = y;

  vec2_normalize_i(&od->position_orientation.orientation_x[obj_idx], &od->position_orientation.orientation_y[obj_idx], 1);
}

static void _ship_handle_collision(entity_id_t id, const message_t* msg) {
  entity_id_t id_a = { (uint32_t)msg->data_a };
  entity_id_t id_b = { (uint32_t)msg->data_b };

  entity_id_t other = (GET_TYPE(id_a) == ENTITY_TYPEREF_SHIP._) ? id_b : id_a;
  uint8_t other_type = GET_TYPE(other);

  // Sun collision is handled in collisions.c (instant burn, no explosion)
  if (other_type == ENTITY_TYPE_SUN) return;

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

static void _ship_dispatch(entity_id_t id, message_t msg) {
  switch (msg.message) {
  case MESSAGE_ROTATE_BY:
    _ship_rotate_by(id, msg.data_a);
    break;
  case MESSAGE_BROADCAST_120HZ_BEFORE_PHYSICS:
    _ship_apply_thrust_from_engines();
    break;
  case MESSAGE_COLLIDE_OBJECT_OBJECT:
    _ship_handle_collision(id, &msg);
    break;
  case MESSAGE_ROTATE_TO:
    _ship_rotate_to(id, _i2f(msg.data_a), _i2f(msg.data_b));
    break;
  case MESSAGE_AI_ORBIT_DISENGAGE:
    ai_orbit_disengage(id);
    break;
  }
}

void ship_entity_initialize(void) {
  entity_manager_vtables[ENTITY_TYPE_SHIP].dispatch_message = _ship_dispatch;
}


#include "platform/platform.h"
#include "platform/math.h"

#include "entity_internal.h"
#include "entity.h"
#include "engine.h"
#include "ship.h"

#include <math.h>

#define MAXSIZE 8192

// === Orbit autopilot ===

#define GRAVITATIONAL_CONSTANT 6.67430f

#define ORBIT_ENGAGE_DELAY_TICKS 36     // 0.3s of stable conditions before engaging
#define ORBIT_MIN_DISTANCE_FACTOR 1.5f  // minimum distance from planet surface
#define ORBIT_MAX_DISTANCE_FACTOR 8.0f  // max detection range (multiples of planet radius)
#define ORBIT_TANGENTIAL_THRESHOLD 0.4f // max |v_radial| / |v| to consider tangential
#define ORBIT_CORRECTION_RATE 0.08f     // velocity blend per physics tick

struct orbit_state {
  uint32_t ship_ordinal;
  uint32_t planet_ordinal;
  int engage_counter;
  bool active;
};

static struct orbit_state _orbit = {
  .ship_ordinal = UINT32_MAX,
  .planet_ordinal = UINT32_MAX,
  .engage_counter = 0,
  .active = false
};

static void _orbit_disengage(void) {
  if (_orbit.active) {
    _orbit.active = false;
    messaging_send(RECIPIENT_ID_BROADCAST, CREATE_MESSAGE(MESSAGE_SHIP_AUTOPILOT_DISENGAGE, 0, 0));
  }
  _orbit.engage_counter = 0;
  _orbit.ship_ordinal = UINT32_MAX;
  _orbit.planet_ordinal = UINT32_MAX;
}

// Check orbit conditions for a ship with zero thrust
static void _ship_check_orbit_conditions(struct objects_data* od, uint32_t ship_idx) {
  float sx = od->position_orientation.position_x[ship_idx];
  float sy = od->position_orientation.position_y[ship_idx];
  float svx = od->velocity_x[ship_idx];
  float svy = od->velocity_y[ship_idx];

  float speed_sq = svx * svx + svy * svy;
  if (speed_sq < 0.001f) return; // stationary

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

  // Check orbital zone
  if (dist < planet_radius * ORBIT_MIN_DISTANCE_FACTOR) return;
  if (dist > planet_radius * ORBIT_MAX_DISTANCE_FACTOR) return;

  // Check escape velocity: v < sqrt(2 * G * M / r)
  float planet_mass = od->mass[best_planet];
  float v_escape_sq = 2.0f * GRAVITATIONAL_CONSTANT * planet_mass / dist;
  if (speed_sq > v_escape_sq) return;

  // Check tangential angle
  float rnx = (sx - od->position_orientation.position_x[best_planet]) / dist;
  float rny = (sy - od->position_orientation.position_y[best_planet]) / dist;
  float v_radial = svx * rnx + svy * rny;

  if (fabsf(v_radial) / speed > ORBIT_TANGENTIAL_THRESHOLD) return;

  // All conditions met - track this ship/planet pair
  if (_orbit.ship_ordinal == ship_idx && _orbit.planet_ordinal == best_planet) {
    _orbit.engage_counter++;
  } else {
    _orbit.ship_ordinal = ship_idx;
    _orbit.planet_ordinal = best_planet;
    _orbit.engage_counter = 1;
  }

  if (_orbit.engage_counter >= ORBIT_ENGAGE_DELAY_TICKS && !_orbit.active) {
    _orbit.active = true;
    messaging_send(RECIPIENT_ID_BROADCAST, CREATE_MESSAGE(MESSAGE_SHIP_AUTOPILOT_ENGAGE, 0, 0));
  }
}

static void _ship_orbit_autopilot_tick(struct objects_data* od) {
  if (!_orbit.active) return;

  uint32_t si = _orbit.ship_ordinal;
  uint32_t pi = _orbit.planet_ordinal;

  // Safety checks
  if (si >= od->active || pi >= od->active) { _orbit_disengage(); return; }
  if (od->type[pi]._ != ENTITY_TYPE_PLANET) { _orbit_disengage(); return; }
  if (od->type[si]._ != ENTITY_TYPE_SHIP) { _orbit_disengage(); return; }

  float sx = od->position_orientation.position_x[si];
  float sy = od->position_orientation.position_y[si];
  float px = od->position_orientation.position_x[pi];
  float py = od->position_orientation.position_y[pi];

  // Radial vector (planet to ship)
  float rx = sx - px;
  float ry = sy - py;
  float dist_sq = rx * rx + ry * ry;

  if (dist_sq < 1.0f) { _orbit_disengage(); return; }

  float dist = sqrtf(dist_sq);
  float rnx = rx / dist;
  float rny = ry / dist;

  // Pick tangent direction closest to current velocity
  float svx = od->velocity_x[si];
  float svy = od->velocity_y[si];

  // Two possible tangents: CCW (-rny, rnx) or CW (rny, -rnx)
  float dot_ccw = svx * (-rny) + svy * rnx;
  float dot_cw = svx * rny + svy * (-rnx);

  float tx, ty;
  if (dot_ccw >= dot_cw) {
    tx = -rny; ty = rnx;
  } else {
    tx = rny; ty = -rnx;
  }

  // Orbital velocity: v = sqrt(G * M / r)
  float planet_mass = od->mass[pi];
  float v_orbit = sqrtf(GRAVITATIONAL_CONSTANT * planet_mass / dist);

  // Target velocity
  float target_vx = tx * v_orbit;
  float target_vy = ty * v_orbit;

  // Blend toward target
  od->velocity_x[si] += (target_vx - svx) * ORBIT_CORRECTION_RATE;
  od->velocity_y[si] += (target_vy - svy) * ORBIT_CORRECTION_RATE;

  // Orient ship in direction of movement
  od->position_orientation.orientation_x[si] = tx;
  od->position_orientation.orientation_y[si] = ty;
}

// === Ship core logic ===

static void _ship_rotate_by(entity_id_t idx, int32_t rotation) {
  // Cancel autopilot on manual rotation
  if (_orbit.active && GET_ORDINAL(idx) == _orbit.ship_ordinal) {
    _orbit_disengage();
  }

  uint32_t id = GET_ORDINAL(idx);
  struct objects_data* od = entity_manager_get_objects();

  float ox = od->position_orientation.orientation_x[id];
  float oy = od->position_orientation.orientation_y[id];

  // 2D rotation matrix: [cos -sin] [x]   [x*cos - y*sin]
  //                     [sin  cos] [y] = [x*sin + y*cos]
  float c = lut_cos(rotation);
  float s = lut_sin(rotation);

  od->position_orientation.orientation_x[id] = ox * c - oy * s;
  od->position_orientation.orientation_y[id] = ox * s + oy * c;

  // renormalize to avoid drift over many rotations
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

  // Orbit detection and autopilot
  for (size_t i = 0; i < od->active; i++) {
    if (od->type[i]._ != ENTITY_TYPE_SHIP) continue;

    if (od->thrust[i] > 0.0f) {
      // Player is thrusting - cancel autopilot if this ship is orbiting
      if (_orbit.ship_ordinal == (uint32_t)i) {
        _orbit_disengage();
      }
    } else if (!_orbit.active) {
      // No thrust and no active autopilot - check orbit conditions
      _ship_check_orbit_conditions(od, (uint32_t)i);
    }
  }

  _ship_orbit_autopilot_tick(od);
}

static void _ship_rotate_to(entity_id_t id, float x, float y) {
  // Cancel autopilot on manual rotate-to (SPACE key)
  if (_orbit.active && GET_ORDINAL(id) == _orbit.ship_ordinal) {
    _orbit_disengage();
  }

  uint32_t obj_idx = GET_ORDINAL(id);
  struct objects_data* od = entity_manager_get_objects();

  od->position_orientation.orientation_x[obj_idx] = x;
  od->position_orientation.orientation_y[obj_idx] = y;

  // renormalize to avoid drift
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
  }
}

void ship_entity_initialize(void) {
  entity_manager_vtables[ENTITY_TYPE_SHIP].dispatch_message = _ship_dispatch;
}

void ship_remap_orbit(uint32_t* remap, uint32_t old_active) {
  if (!_orbit.active && _orbit.engage_counter == 0) return;

  bool valid = true;

  if (_orbit.ship_ordinal < old_active) {
    uint32_t new_ord = remap[_orbit.ship_ordinal];
    if (new_ord != UINT32_MAX) {
      _orbit.ship_ordinal = new_ord;
    } else {
      valid = false;
    }
  }

  if (valid && _orbit.planet_ordinal < old_active) {
    uint32_t new_ord = remap[_orbit.planet_ordinal];
    if (new_ord != UINT32_MAX) {
      _orbit.planet_ordinal = new_ord;
    } else {
      valid = false;
    }
  }

  if (!valid) {
    _orbit_disengage();
  }
}

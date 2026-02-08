#include "autopilot.h"
#include "entity.h"
#include "messaging/messaging.h"
#include "platform/platform.h"

#include <math.h>

#define MAX_AUTOPILOT_SLOTS 8

#define GRAVITATIONAL_CONSTANT 6.67430f

// Hysteresis: start correcting above HIGH, stop correcting below LOW.
// The gap prevents jitter near the boundary.
#define DV_CORRECT_ENTER 3.0f
#define DV_CORRECT_EXIT 0.8f

// During coast: only re-evaluate delta-v every N ticks (0.1s)
#define COAST_EVAL_INTERVAL 12
// During coast: update ship orientation every N ticks
#define COAST_ORIENT_INTERVAL 6
// Thrust proportional gain: thrust_pct = delta_v * scale, clamped to [1, 100]
#define THRUST_GAIN 0.4f

// Collision safety
#define EMERGENCY_DISTANCE_FACTOR 1.2f  // fire outward if closer than 1.2x planet radius
#define EMERGENCY_LOOKAHEAD_S 1.0f      // predict collision 1 second ahead
#define RADIAL_SAFETY_GAIN 1.5f         // boost delta-v outward when approaching planet

enum autopilot_phase {
  AP_COAST,
  AP_CORRECT
};

struct autopilot_slot {
  bool active;
  uint32_t ship_ordinal;
  uint32_t planet_ordinal;
  enum autopilot_phase phase;
  int timer;
};

static struct autopilot_slot _slots[MAX_AUTOPILOT_SLOTS];

void autopilot_initialize(void) {
  for (int i = 0; i < MAX_AUTOPILOT_SLOTS; i++) {
    _slots[i].active = false;
  }
}

static int _find_slot_for_ship(uint32_t ship_ordinal) {
  for (int i = 0; i < MAX_AUTOPILOT_SLOTS; i++) {
    if (_slots[i].active && _slots[i].ship_ordinal == ship_ordinal) {
      return i;
    }
  }
  return -1;
}

static int _find_free_slot(void) {
  for (int i = 0; i < MAX_AUTOPILOT_SLOTS; i++) {
    if (!_slots[i].active) return i;
  }
  return -1;
}

static void _slot_disengage(int idx) {
  struct autopilot_slot* s = &_slots[idx];
  if (!s->active) return;

  // Engines off
  entity_id_t ship_id = OBJECT_ID_WITH_TYPE(s->ship_ordinal, ENTITY_TYPE_SHIP);
  messaging_send(PARTS_OF_TYPE(ship_id, PART_TYPEREF_ENGINE),
                 CREATE_MESSAGE(MESSAGE_SHIP_ENGINES_THRUST, 0, 0));

  s->active = false;
  messaging_send(RECIPIENT_ID_BROADCAST, CREATE_MESSAGE(MESSAGE_SHIP_AUTOPILOT_DISENGAGE, 0, 0));
}

// Compute orbital target velocity for a ship around a planet.
// Returns delta-v magnitude. Writes target tangent into tx/ty.
static float _compute_orbit_dv(struct objects_data* od, uint32_t si, uint32_t pi,
                               float* out_dvx, float* out_dvy, float* out_tx, float* out_ty) {
  float sx = od->position_orientation.position_x[si];
  float sy = od->position_orientation.position_y[si];
  float px = od->position_orientation.position_x[pi];
  float py = od->position_orientation.position_y[pi];

  float rx = sx - px;
  float ry = sy - py;
  float dist = sqrtf(rx * rx + ry * ry);

  if (dist < 1.0f) {
    *out_dvx = 0; *out_dvy = 0;
    *out_tx = 1; *out_ty = 0;
    return 0;
  }

  float rnx = rx / dist;
  float rny = ry / dist;

  float svx = od->velocity_x[si];
  float svy = od->velocity_y[si];

  // Pick tangent direction closest to current velocity
  float dot_ccw = svx * (-rny) + svy * rnx;
  float dot_cw = svx * rny + svy * (-rnx);

  float tx, ty;
  if (dot_ccw >= dot_cw) {
    tx = -rny; ty = rnx;
  } else {
    tx = rny; ty = -rnx;
  }

  *out_tx = tx;
  *out_ty = ty;

  float v_orbit = sqrtf(GRAVITATIONAL_CONSTANT * od->mass[pi] / dist);

  float target_vx = tx * v_orbit;
  float target_vy = ty * v_orbit;

  float dvx = target_vx - svx;
  float dvy = target_vy - svy;

  *out_dvx = dvx;
  *out_dvy = dvy;

  return sqrtf(dvx * dvx + dvy * dvy);
}

static void _slot_tick(struct autopilot_slot* s) {
  struct objects_data* od = entity_manager_get_objects();

  // Safety: validate indices
  if (s->ship_ordinal >= od->active || s->planet_ordinal >= od->active) {
    _slot_disengage((int)(s - _slots));
    return;
  }
  if (od->type[s->planet_ordinal]._ != ENTITY_TYPE_PLANET ||
      od->type[s->ship_ordinal]._ != ENTITY_TYPE_SHIP) {
    _slot_disengage((int)(s - _slots));
    return;
  }

  entity_id_t ship_id = OBJECT_ID_WITH_TYPE(s->ship_ordinal, ENTITY_TYPE_SHIP);

  // --- Collision safety (every tick, regardless of phase) ---
  float sx = od->position_orientation.position_x[s->ship_ordinal];
  float sy = od->position_orientation.position_y[s->ship_ordinal];
  float px = od->position_orientation.position_x[s->planet_ordinal];
  float py = od->position_orientation.position_y[s->planet_ordinal];
  float rx = sx - px;
  float ry = sy - py;
  float dist = sqrtf(rx * rx + ry * ry);
  float planet_radius = od->position_orientation.radius[s->planet_ordinal];

  if (dist < 1.0f) { _slot_disengage((int)(s - _slots)); return; }

  float rnx = rx / dist;
  float rny = ry / dist;
  float svx = od->velocity_x[s->ship_ordinal];
  float svy = od->velocity_y[s->ship_ordinal];
  float v_radial = svx * rnx + svy * rny; // positive = away, negative = approaching

  float emergency_dist = planet_radius * EMERGENCY_DISTANCE_FACTOR;
  bool too_close = dist < emergency_dist;
  bool impact_imminent = (v_radial < -1.0f) &&
                         (dist + v_radial * EMERGENCY_LOOKAHEAD_S) < emergency_dist;

  if (too_close || impact_imminent) {
    // EMERGENCY: fire radially OUTWARD at full thrust
    s->phase = AP_CORRECT;
    messaging_send(ship_id, CREATE_MESSAGE_F(MESSAGE_SHIP_ROTATE_TO, rnx, rny));
    messaging_send(PARTS_OF_TYPE(ship_id, PART_TYPEREF_ENGINE),
                   CREATE_MESSAGE(MESSAGE_SHIP_ENGINES_THRUST, 100, 0));
    return;
  }

  // If approaching planet during coast, break out into correction early
  if (v_radial < -2.0f && s->phase == AP_COAST) {
    s->phase = AP_CORRECT;
  }

  // --- Normal phase logic ---
  float dvx, dvy, tx, ty;

  switch (s->phase) {

  case AP_CORRECT: {
    float dv = _compute_orbit_dv(od, s->ship_ordinal, s->planet_ordinal, &dvx, &dvy, &tx, &ty);

    // Radial safety boost: if approaching planet, bias correction outward
    if (v_radial < 0) {
      float boost = (-v_radial) * RADIAL_SAFETY_GAIN;
      dvx += rnx * boost;
      dvy += rny * boost;
      dv = sqrtf(dvx * dvx + dvy * dvy);
    }

    if (dv < DV_CORRECT_EXIT && v_radial > -1.0f) {
      // Orbit close enough and not approaching — coast
      s->phase = AP_COAST;
      s->timer = COAST_EVAL_INTERVAL;

      messaging_send(PARTS_OF_TYPE(ship_id, PART_TYPEREF_ENGINE),
                     CREATE_MESSAGE(MESSAGE_SHIP_ENGINES_THRUST, 0, 0));
      messaging_send(ship_id, CREATE_MESSAGE_F(MESSAGE_SHIP_ROTATE_TO, tx, ty));
      break;
    }

    // Proportional correction: rotate toward delta-v, fire engines
    float dvnx = dvx / dv;
    float dvny = dvy / dv;
    messaging_send(ship_id, CREATE_MESSAGE_F(MESSAGE_SHIP_ROTATE_TO, dvnx, dvny));

    int thrust_pct = (int)(dv * THRUST_GAIN);
    if (thrust_pct < 1) thrust_pct = 1;
    if (thrust_pct > 100) thrust_pct = 100;

    messaging_send(PARTS_OF_TYPE(ship_id, PART_TYPEREF_ENGINE),
                   CREATE_MESSAGE(MESSAGE_SHIP_ENGINES_THRUST, thrust_pct, 0));
    break;
  }

  case AP_COAST: {
    s->timer--;

    // Periodically orient along tangent
    if (s->timer % COAST_ORIENT_INTERVAL == 0) {
      _compute_orbit_dv(od, s->ship_ordinal, s->planet_ordinal, &dvx, &dvy, &tx, &ty);
      messaging_send(ship_id, CREATE_MESSAGE_F(MESSAGE_SHIP_ROTATE_TO, tx, ty));
    }

    // Periodic evaluation
    if (s->timer <= 0) {
      float dv = _compute_orbit_dv(od, s->ship_ordinal, s->planet_ordinal, &dvx, &dvy, &tx, &ty);

      if (dv > DV_CORRECT_ENTER) {
        s->phase = AP_CORRECT;
      } else {
        s->timer = COAST_EVAL_INTERVAL;
      }
    }
    break;
  }
  }
}

// === Public API ===

void autopilot_orbit_engage(entity_id_t ship_id, uint32_t planet_ordinal) {
  uint32_t ord = GET_ORDINAL(ship_id);

  int idx = _find_slot_for_ship(ord);
  if (idx >= 0) {
    // Already active — update planet target
    _slots[idx].planet_ordinal = planet_ordinal;
    return;
  }

  idx = _find_free_slot();
  if (idx < 0) return; // no free slots

  _slots[idx].active = true;
  _slots[idx].ship_ordinal = ord;
  _slots[idx].planet_ordinal = planet_ordinal;
  _slots[idx].phase = AP_CORRECT;
  _slots[idx].timer = 0;

  messaging_send(RECIPIENT_ID_BROADCAST, CREATE_MESSAGE(MESSAGE_SHIP_AUTOPILOT_ENGAGE, 0, 0));
}

void autopilot_orbit_disengage(entity_id_t ship_id) {
  int idx = _find_slot_for_ship(GET_ORDINAL(ship_id));
  if (idx >= 0) {
    _slot_disengage(idx);
  }
}

bool autopilot_orbit_is_active(entity_id_t ship_id) {
  return _find_slot_for_ship(GET_ORDINAL(ship_id)) >= 0;
}

void autopilot_tick(void) {
  for (int i = 0; i < MAX_AUTOPILOT_SLOTS; i++) {
    if (_slots[i].active) {
      _slot_tick(&_slots[i]);
    }
  }
}

void autopilot_remap(uint32_t* remap, uint32_t old_active) {
  for (int i = 0; i < MAX_AUTOPILOT_SLOTS; i++) {
    if (!_slots[i].active) continue;

    bool valid = true;

    if (_slots[i].ship_ordinal < old_active) {
      uint32_t new_ord = remap[_slots[i].ship_ordinal];
      if (new_ord != UINT32_MAX)
        _slots[i].ship_ordinal = new_ord;
      else
        valid = false;
    }

    if (valid && _slots[i].planet_ordinal < old_active) {
      uint32_t new_ord = remap[_slots[i].planet_ordinal];
      if (new_ord != UINT32_MAX)
        _slots[i].planet_ordinal = new_ord;
      else
        valid = false;
    }

    if (!valid) {
      // Don't call _slot_disengage here — ordinals are stale after pack,
      // sending messages with them would target wrong entities.
      _slots[i].active = false;
      messaging_send(RECIPIENT_ID_BROADCAST, CREATE_MESSAGE(MESSAGE_SHIP_AUTOPILOT_DISENGAGE, 0, 0));
    }
  }
}

void autopilot_query_status(entity_id_t ship_id, autopilot_status_t* out) {
  out->active = false;
  out->phase = 0;
  out->delta_v = 0.0f;
  out->v_orbit = 0.0f;

  int idx = _find_slot_for_ship(GET_ORDINAL(ship_id));
  if (idx < 0) return;

  struct autopilot_slot* s = &_slots[idx];
  if (!s->active) return;

  out->active = true;
  out->phase = (s->phase == AP_CORRECT) ? 1 : 0;

  struct objects_data* od = entity_manager_get_objects();
  if (s->ship_ordinal >= od->active || s->planet_ordinal >= od->active) return;

  float dvx, dvy, tx, ty;
  out->delta_v = _compute_orbit_dv(od, s->ship_ordinal, s->planet_ordinal, &dvx, &dvy, &tx, &ty);

  float sx = od->position_orientation.position_x[s->ship_ordinal];
  float sy = od->position_orientation.position_y[s->ship_ordinal];
  float px = od->position_orientation.position_x[s->planet_ordinal];
  float py = od->position_orientation.position_y[s->planet_ordinal];
  float rx = sx - px;
  float ry = sy - py;
  float dist = sqrtf(rx * rx + ry * ry);
  if (dist < 1.0f) dist = 1.0f;
  out->v_orbit = sqrtf(GRAVITATIONAL_CONSTANT * od->mass[s->planet_ordinal] / dist);
}

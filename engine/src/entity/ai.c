#include "ai.h"
#include "entity.h"
#include "radar.h"
#include "messaging/messaging.h"
#include "platform/platform.h"
#include "core/core.h"

#define MAX_AI_SLOTS 16

#define GRAVITATIONAL_CONSTANT 6.67430f

// Hysteresis: start correcting above HIGH, stop correcting below LOW.
// The gap prevents jitter near the boundary.
#define DV_CORRECT_ENTER 3.0f
#define DV_CORRECT_EXIT 0.8f
#define DV_CORRECT_ENTER_SQ (DV_CORRECT_ENTER * DV_CORRECT_ENTER)  // 9.0f
#define DV_CORRECT_EXIT_SQ  (DV_CORRECT_EXIT  * DV_CORRECT_EXIT)   // 0.64f

// During coast: only re-evaluate delta-v every N ticks (0.1s)
#define COAST_EVAL_INTERVAL 12
// During coast: update entity orientation every N ticks
#define COAST_ORIENT_INTERVAL 6
// Thrust proportional gain: thrust_pct = delta_v * scale, clamped to [1, 100]
#define THRUST_GAIN 0.4f

// Collision safety
#define EMERGENCY_DISTANCE_FACTOR 1.2f  // fire outward if closer than 1.2x body radius
#define EMERGENCY_LOOKAHEAD_S 1.0f      // predict collision 1 second ahead
#define RADIAL_SAFETY_GAIN 1.5f         // boost delta-v outward when approaching body

enum ai_phase {
  AI_COAST,
  AI_CORRECT
};

struct ai_slot {
  bool active;
  uint32_t entity_ordinal;
  uint8_t entity_type;       // stored so we can reconstruct entity_id after remap
  uint32_t body_ordinal;
  enum ai_phase phase;
  int timer;
};

static struct ai_slot _slots[MAX_AI_SLOTS];

void ai_initialize(void) {
  for (int i = 0; i < MAX_AI_SLOTS; i++) {
    _slots[i].active = false;
  }
}

static int _find_slot_for_entity(uint32_t entity_ordinal) {
  for (int i = 0; i < MAX_AI_SLOTS; i++) {
    if (_slots[i].active && _slots[i].entity_ordinal == entity_ordinal) {
      return i;
    }
  }
  return -1;
}

static int _find_free_slot(void) {
  for (int i = 0; i < MAX_AI_SLOTS; i++) {
    if (!_slots[i].active) return i;
  }
  return -1;
}

static entity_id_t _slot_entity_id(const struct ai_slot* s) {
  return OBJECT_ID_WITH_TYPE(s->entity_ordinal, s->entity_type);
}

static void _slot_disengage(int idx) {
  struct ai_slot* s = &_slots[idx];
  if (!s->active) return;

  // Engines off
  entity_id_t eid = _slot_entity_id(s);
  messaging_send(PARTS_OF_TYPE(eid, PART_TYPEREF_ENGINE),
                 CREATE_MESSAGE(MESSAGE_ENGINES_THRUST, 0, 0));

  s->active = false;
  messaging_send(RECIPIENT_ID_BROADCAST, CREATE_MESSAGE(MESSAGE_AI_ORBIT_DISENGAGE, 0, 0));
}

// Compute orbital target velocity for an entity around a body.
// Returns squared delta-v magnitude. Writes target tangent into tx/ty.
static float _compute_orbit_dv_sq(struct objects_data* od, uint32_t ei, uint32_t bi,
                                  float* out_dvx, float* out_dvy, float* out_tx, float* out_ty) {
  float sx = od->position_orientation.position_x[ei];
  float sy = od->position_orientation.position_y[ei];
  float px = od->position_orientation.position_x[bi];
  float py = od->position_orientation.position_y[bi];

  float rx = sx - px;
  float ry = sy - py;
  float dist_sq = rx * rx + ry * ry;

  if (dist_sq < 1.0f) {
    *out_dvx = 0; *out_dvy = 0;
    *out_tx = 1; *out_ty = 0;
    return 0;
  }

  float inv_dist = Q_rsqrt(dist_sq);
  float rnx = rx * inv_dist;
  float rny = ry * inv_dist;

  float svx = od->velocity_x[ei];
  float svy = od->velocity_y[ei];

  // Use velocity RELATIVE to the body for tangent direction selection.
  // The body moves (e.g. planet orbiting sun) — inertial velocity is dominated
  // by the body's motion, not the local orbital direction.
  float rel_vx = svx - od->velocity_x[bi];
  float rel_vy = svy - od->velocity_y[bi];

  // Pick tangent direction closest to relative velocity (CW vs CCW orbit)
  float dot_ccw = rel_vx * (-rny) + rel_vy * rnx;
  float dot_cw = rel_vx * rny + rel_vy * (-rnx);

  float tx, ty;
  if (dot_ccw >= dot_cw) {
    tx = -rny; ty = rnx;
  } else {
    tx = rny; ty = -rnx;
  }

  *out_tx = tx;
  *out_ty = ty;

  // v_orbit^2 = G*M/dist = G*M * inv_dist
  float v_orbit_sq = GRAVITATIONAL_CONSTANT * od->mass[bi] * inv_dist;
  // v_orbit = sqrt(v_orbit_sq) = v_orbit_sq * rsqrt(v_orbit_sq)
  float v_orbit = v_orbit_sq * Q_rsqrt(v_orbit_sq);

  // Target velocity = tangential orbit velocity + body's inertial velocity
  // (the body itself is moving, e.g. planet orbiting the sun)
  float target_vx = tx * v_orbit + od->velocity_x[bi];
  float target_vy = ty * v_orbit + od->velocity_y[bi];

  float dvx = target_vx - svx;
  float dvy = target_vy - svy;

  *out_dvx = dvx;
  *out_dvy = dvy;

  return dvx * dvx + dvy * dvy;
}

// Check if a body ordinal is visible on an entity's radar (nav or tactical contacts).
// Returns true if the entity has no radar (fallback — no sensor means no restriction).
static bool _body_on_radar(entity_id_t owner, uint32_t body_ordinal) {
  const struct radar_data* rd = radar_get_data(owner);
  if (!rd) return true;  // no radar = no sensor restriction

  for (uint8_t i = 0; i < rd->nav_count; i++) {
    if ((uint32_t)rd->nav[i].entity_ordinal == body_ordinal) return true;
  }
  for (uint8_t i = 0; i < rd->contact_count; i++) {
    if ((uint32_t)rd->contacts[i].entity_ordinal == body_ordinal) return true;
  }
  return false;
}

static void _slot_tick(struct ai_slot* s) {
  struct objects_data* od = entity_manager_get_objects();

  // Safety: validate indices
  if (s->entity_ordinal >= od->active || s->body_ordinal >= od->active) {
    _slot_disengage((int)(s - _slots));
    return;
  }

  entity_id_t eid = _slot_entity_id(s);

  // Validate body is still visible on radar.
  // If the entity has a radar and the body is not on it, disengage.
  if (!_body_on_radar(eid, s->body_ordinal)) {
    _slot_disengage((int)(s - _slots));
    return;
  }

  // --- Collision safety (every tick, regardless of phase) ---
  float sx = od->position_orientation.position_x[s->entity_ordinal];
  float sy = od->position_orientation.position_y[s->entity_ordinal];
  float px = od->position_orientation.position_x[s->body_ordinal];
  float py = od->position_orientation.position_y[s->body_ordinal];
  float rx = sx - px;
  float ry = sy - py;
  float dist_sq = rx * rx + ry * ry;
  float body_radius = od->position_orientation.radius[s->body_ordinal];

  if (dist_sq < 1.0f) { _slot_disengage((int)(s - _slots)); return; }

  float inv_dist = Q_rsqrt(dist_sq);
  float dist = dist_sq * inv_dist;  // ≈ sqrt(dist_sq)
  float rnx = rx * inv_dist;
  float rny = ry * inv_dist;
  // Velocity RELATIVE to the body for all radial safety checks.
  // The body moves (e.g. planet orbiting sun at ~115 u/s);
  // using inertial velocity would see phantom "approaching" at that speed.
  float rel_vx = od->velocity_x[s->entity_ordinal] - od->velocity_x[s->body_ordinal];
  float rel_vy = od->velocity_y[s->entity_ordinal] - od->velocity_y[s->body_ordinal];
  float v_radial = rel_vx * rnx + rel_vy * rny; // positive = away, negative = approaching

  float emergency_dist = body_radius * EMERGENCY_DISTANCE_FACTOR;
  bool too_close = dist < emergency_dist;
  bool impact_imminent = (v_radial < -1.0f) &&
                         (dist + v_radial * EMERGENCY_LOOKAHEAD_S) < emergency_dist;

  if (too_close || impact_imminent) {
    // EMERGENCY: fire radially OUTWARD at full thrust
    s->phase = AI_CORRECT;
    messaging_send(eid, CREATE_MESSAGE_F(MESSAGE_ROTATE_TO, rnx, rny));
    messaging_send(PARTS_OF_TYPE(eid, PART_TYPEREF_ENGINE),
                   CREATE_MESSAGE(MESSAGE_ENGINES_THRUST, 100, 0));
    return;
  }

  // If approaching body during coast, break out into correction early
  if (v_radial < -2.0f && s->phase == AI_COAST) {
    s->phase = AI_CORRECT;
  }

  // --- Normal phase logic ---
  float dvx, dvy, tx, ty;

  switch (s->phase) {

  case AI_CORRECT: {
    float dv_sq = _compute_orbit_dv_sq(od, s->entity_ordinal, s->body_ordinal, &dvx, &dvy, &tx, &ty);

    // Radial safety boost: if approaching body, bias correction outward
    if (v_radial < 0) {
      float boost = (-v_radial) * RADIAL_SAFETY_GAIN;
      dvx += rnx * boost;
      dvy += rny * boost;
      dv_sq = dvx * dvx + dvy * dvy;
    }

    if (dv_sq < DV_CORRECT_EXIT_SQ && v_radial > -1.0f) {
      // Orbit close enough and not approaching — coast
      s->phase = AI_COAST;
      s->timer = COAST_EVAL_INTERVAL;

      messaging_send(PARTS_OF_TYPE(eid, PART_TYPEREF_ENGINE),
                     CREATE_MESSAGE(MESSAGE_ENGINES_THRUST, 0, 0));
      // Don't override orientation during coast — let player rotate freely
      break;
    }

    // Proportional correction: rotate toward delta-v, fire engines
    float inv_dv = Q_rsqrt(dv_sq);
    float dvnx = dvx * inv_dv;
    float dvny = dvy * inv_dv;
    messaging_send(eid, CREATE_MESSAGE_F(MESSAGE_ROTATE_TO, dvnx, dvny));

    // dv = dv_sq * inv_dv  (= sqrt(dv_sq))
    int thrust_pct = (int)(dv_sq * inv_dv * THRUST_GAIN);
    if (thrust_pct < 1) thrust_pct = 1;
    if (thrust_pct > 100) thrust_pct = 100;

    messaging_send(PARTS_OF_TYPE(eid, PART_TYPEREF_ENGINE),
                   CREATE_MESSAGE(MESSAGE_ENGINES_THRUST, thrust_pct, 0));
    break;
  }

  case AI_COAST: {
    s->timer--;

    // No orientation override during coast — player has full rotation control

    // Periodic evaluation
    if (s->timer <= 0) {
      float dv_sq = _compute_orbit_dv_sq(od, s->entity_ordinal, s->body_ordinal, &dvx, &dvy, &tx, &ty);

      if (dv_sq > DV_CORRECT_ENTER_SQ) {
        s->phase = AI_CORRECT;
      } else {
        s->timer = COAST_EVAL_INTERVAL;
      }
    }
    break;
  }
  }
}

// === Public API ===

void ai_orbit_engage(entity_id_t entity_id, uint32_t body_ordinal) {
  uint32_t ord = GET_ORDINAL(entity_id);

  int idx = _find_slot_for_entity(ord);
  if (idx >= 0) {
    // Already active — update body target
    _slots[idx].body_ordinal = body_ordinal;
    return;
  }

  idx = _find_free_slot();
  if (idx < 0) return; // no free slots

  _slots[idx].active = true;
  _slots[idx].entity_ordinal = ord;
  _slots[idx].entity_type = GET_TYPE(entity_id);
  _slots[idx].body_ordinal = body_ordinal;
  _slots[idx].phase = AI_CORRECT;
  _slots[idx].timer = 0;

  messaging_send(RECIPIENT_ID_BROADCAST, CREATE_MESSAGE(MESSAGE_AI_ORBIT_ENGAGE, 0, 0));
}

void ai_orbit_disengage(entity_id_t entity_id) {
  int idx = _find_slot_for_entity(GET_ORDINAL(entity_id));
  if (idx >= 0) {
    _slot_disengage(idx);
  }
}

bool ai_orbit_is_active(entity_id_t entity_id) {
  return _find_slot_for_entity(GET_ORDINAL(entity_id)) >= 0;
}

void ai_tick(void) {
  for (int i = 0; i < MAX_AI_SLOTS; i++) {
    if (_slots[i].active) {
      _slot_tick(&_slots[i]);
    }
  }
}

void ai_remap(uint32_t* remap, uint32_t old_active) {
  for (int i = 0; i < MAX_AI_SLOTS; i++) {
    if (!_slots[i].active) continue;

    bool valid = true;

    if (_slots[i].entity_ordinal < old_active) {
      uint32_t new_ord = remap[_slots[i].entity_ordinal];
      if (new_ord != UINT32_MAX)
        _slots[i].entity_ordinal = new_ord;
      else
        valid = false;
    }

    if (valid && _slots[i].body_ordinal < old_active) {
      uint32_t new_ord = remap[_slots[i].body_ordinal];
      if (new_ord != UINT32_MAX)
        _slots[i].body_ordinal = new_ord;
      else
        valid = false;
    }

    if (!valid) {
      // Don't call _slot_disengage here — ordinals are stale after pack,
      // sending messages with them would target wrong entities.
      _slots[i].active = false;
      messaging_send(RECIPIENT_ID_BROADCAST, CREATE_MESSAGE(MESSAGE_AI_ORBIT_DISENGAGE, 0, 0));
    }
  }
}

void ai_query_status(entity_id_t entity_id, ai_status_t* out) {
  out->active = false;
  out->phase = 0;
  out->delta_v = 0.0f;
  out->v_orbit = 0.0f;

  int idx = _find_slot_for_entity(GET_ORDINAL(entity_id));
  if (idx < 0) return;

  struct ai_slot* s = &_slots[idx];
  if (!s->active) return;

  out->active = true;
  out->phase = (s->phase == AI_CORRECT) ? 1 : 0;

  struct objects_data* od = entity_manager_get_objects();
  if (s->entity_ordinal >= od->active || s->body_ordinal >= od->active) return;

  float dvx, dvy, tx, ty;
  float dv_sq = _compute_orbit_dv_sq(od, s->entity_ordinal, s->body_ordinal, &dvx, &dvy, &tx, &ty);
  out->delta_v = dv_sq * Q_rsqrt(dv_sq);  // ≈ sqrt(dv_sq)

  float sx = od->position_orientation.position_x[s->entity_ordinal];
  float sy = od->position_orientation.position_y[s->entity_ordinal];
  float px = od->position_orientation.position_x[s->body_ordinal];
  float py = od->position_orientation.position_y[s->body_ordinal];
  float rx = sx - px;
  float ry = sy - py;
  float dist_sq = rx * rx + ry * ry;
  if (dist_sq < 1.0f) dist_sq = 1.0f;
  float inv_dist = Q_rsqrt(dist_sq);
  // v_orbit = sqrt(G*M/dist) = sqrt(G*M * inv_dist)
  float v_orbit_sq = GRAVITATIONAL_CONSTANT * od->mass[s->body_ordinal] * inv_dist;
  out->v_orbit = v_orbit_sq * Q_rsqrt(v_orbit_sq);
}

#include "platform/platform.h"
#include "platform/math.h"
#include "rocket.h"
#include "particles.h"
#include "explosion.h"
#include "../generated/renderer.gen.h"

#define ROCKET_AUX_CAPACITY 256
#define SMOKE_SPAWN_CHANCE 0.3f
#define ROCKET_MASS 0.1f
#define ROCKET_RADIUS 4.0f
#define ROCKET_GRACE_TICKS 15 // ~0.125s invulnerability after spawn to clear parent ship

// SoA auxiliary data for rockets (physics lives in objects_data)
static struct {
  uint32_t count;
  uint32_t capacity;
  uint32_t* object_idx;
  uint16_t* smoke_model;
  uint16_t* fuel_ticks;
  uint16_t* lifetime_ticks;
  uint16_t* grace_ticks; // collision grace period after spawn
  float* thrust_power;
} rocket_aux_;

void rocket_create(float x, float y, float vx, float vy, float ox, float oy,
                   uint16_t model_idx, uint16_t smoke_model_idx,
                   float thrust_power, uint16_t fuel_ticks, uint16_t lifetime_ticks) {
  if (rocket_aux_.count >= rocket_aux_.capacity) return;

  struct objects_data* od = entity_manager_get_objects();
  if (od->active >= od->capacity) return;

  // Add rocket as a regular object -> gets physics (gravity, integration) and collisions for free
  uint32_t idx = od->active++;
  od->type[idx] = ENTITY_TYPEREF_ROCKET;
  od->model_idx[idx] = model_idx;
  od->position_orientation.position_x[idx] = x;
  od->position_orientation.position_y[idx] = y;
  od->position_orientation.orientation_x[idx] = ox;
  od->position_orientation.orientation_y[idx] = oy;
  od->position_orientation.radius[idx] = ROCKET_RADIUS;
  od->velocity_x[idx] = vx;
  od->velocity_y[idx] = vy;
  od->acceleration_x[idx] = 0.0f;
  od->acceleration_y[idx] = 0.0f;
  od->thrust[idx] = thrust_power; // ignite motor immediately
  od->mass[idx] = ROCKET_MASS;
  od->parts_start_idx[idx] = 0;
  od->parts_count[idx] = 0;
  od->health[idx] = 1; // alive, sweep won't touch it until <= 0

  // Store auxiliary data (smoke, fuel, lifetime) - not needed by physics
  uint32_t ai = rocket_aux_.count++;
  rocket_aux_.object_idx[ai] = idx;
  rocket_aux_.smoke_model[ai] = smoke_model_idx;
  rocket_aux_.fuel_ticks[ai] = fuel_ticks;
  rocket_aux_.lifetime_ticks[ai] = lifetime_ticks;
  rocket_aux_.grace_ticks[ai] = ROCKET_GRACE_TICKS;
  rocket_aux_.thrust_power[ai] = thrust_power;
}

static void _rocket_spawn_smoke(uint32_t ai, struct objects_data* od) {
  uint16_t smoke_model = rocket_aux_.smoke_model[ai];
  if (smoke_model == 0) return;
  if (randf() > SMOKE_SPAWN_CHANCE) return;

  uint32_t idx = rocket_aux_.object_idx[ai];
  float ox = od->position_orientation.orientation_x[idx];
  float oy = od->position_orientation.orientation_y[idx];
  float px = od->position_orientation.position_x[idx];
  float py = od->position_orientation.position_y[idx];

  // perpendicular for spread
  float perp_x = -oy;
  float perp_y = ox;

  // velocity spread (cone behind rocket)
  float spread = randf_symmetric() * 0.2f;
  float speed_variance = 0.5f + randf() * 0.5f;
  float base_speed = 30.0f;

  float svx = (-ox + perp_x * spread) * base_speed * speed_variance;
  float svy = (-oy + perp_y * spread) * base_speed * speed_variance;

  // position behind rocket
  float backward_offset = 5.0f;
  float pos_jitter = 2.0f;
  float spx = px - ox * backward_offset + perp_x * randf_symmetric() * pos_jitter;
  float spy = py - oy * backward_offset + perp_y * randf_symmetric() * pos_jitter;

  // lifetime
  uint16_t ttl = (uint16_t)((0.3f + randf() * 0.5f) * TICKS_IN_SECOND);

  particle_create_t pcm = {
    .x = spx,
    .y = spy,
    .vx = svx,
    .vy = svy,
    .ox = randf_symmetric(),
    .oy = randf_symmetric(),
    .ttl = ttl,
    .model_idx = smoke_model
  };

  particles_create_particle(&pcm);
}

static void _rockets_tick(void) {
  struct objects_data* od = entity_manager_get_objects();

  uint32_t write = 0;
  for (uint32_t i = 0; i < rocket_aux_.count; i++) {
    uint32_t obj_idx = rocket_aux_.object_idx[i];

    // --- Grace period countdown ---
    if (rocket_aux_.grace_ticks[i] > 0) {
      rocket_aux_.grace_ticks[i]--;
    }

    // --- Burning phase: fuel > 0 ---
    if (rocket_aux_.fuel_ticks[i] > 0) {
      rocket_aux_.fuel_ticks[i]--;
      od->thrust[obj_idx] = rocket_aux_.thrust_power[i];
      _rocket_spawn_smoke(i, od);
    } else {
      // --- Coasting phase: no thrust, no smoke ---
      od->thrust[obj_idx] = 0.0f;
    }

    // --- Lifetime ---
    if (rocket_aux_.lifetime_ticks[i] > 0) {
      rocket_aux_.lifetime_ticks[i]--;
      // keep this aux entry (compact in place)
      if (write != i) {
        rocket_aux_.object_idx[write] = rocket_aux_.object_idx[i];
        rocket_aux_.smoke_model[write] = rocket_aux_.smoke_model[i];
        rocket_aux_.fuel_ticks[write] = rocket_aux_.fuel_ticks[i];
        rocket_aux_.lifetime_ticks[write] = rocket_aux_.lifetime_ticks[i];
        rocket_aux_.grace_ticks[write] = rocket_aux_.grace_ticks[i];
        rocket_aux_.thrust_power[write] = rocket_aux_.thrust_power[i];
      }
      write++;
    } else {
      // --- Dead phase: auto-destruct ---
      od->health[obj_idx] = 0; // mark for sweep
    }
  }
  rocket_aux_.count = write;
}

void rocket_remap_objects(uint32_t* remap, uint32_t old_active) {
  for (uint32_t i = 0; i < rocket_aux_.count; i++) {
    uint32_t old_idx = rocket_aux_.object_idx[i];
    if (old_idx < old_active) {
      uint32_t new_idx = remap[old_idx];
      if (new_idx != UINT32_MAX) {
        rocket_aux_.object_idx[i] = new_idx;
      }
      // Note: if new_idx == UINT32_MAX, the object was swept but aux should already
      // have been removed in _rockets_tick (health=0 set before sweep).
      // This case shouldn't happen in normal flow.
    }
  }
}

static void _rocket_handle_collision(entity_id_t id, const message_t* msg) {
  (msg);

  uint32_t self_idx = GET_ORDINAL(id);
  struct objects_data* od = entity_manager_get_objects();

  // already dead?
  if (od->health[self_idx] <= 0) return;

  // find aux entry
  uint32_t ai = UINT32_MAX;
  for (uint32_t i = 0; i < rocket_aux_.count; i++) {
    if (rocket_aux_.object_idx[i] == self_idx) {
      ai = i;
      break;
    }
  }

  // still in grace period? ignore collision (clearing parent ship)
  if (ai != UINT32_MAX && rocket_aux_.grace_ticks[ai] > 0) return;

  // self-destruct: kill the rocket
  od->health[self_idx] = 0;

  // spawn small explosion proportional to rocket mass
  float px = od->position_orientation.position_x[self_idx];
  float py = od->position_orientation.position_y[self_idx];
  float vx = od->velocity_x[self_idx];
  float vy = od->velocity_y[self_idx];

  explosion_spawn(px, py, vx, vy, ROCKET_MASS);

  // kill aux entry so it doesn't keep ticking
  if (ai != UINT32_MAX) {
    rocket_aux_.lifetime_ticks[ai] = 0;
  }
}

static void _rockets_dispatch(entity_id_t id, message_t msg) {
  switch (msg.message) {
    case MESSAGE_BROADCAST_120HZ_AFTER_PHYSICS:
      _rockets_tick();
      break;
    case MESSAGE_COLLIDE_OBJECT_OBJECT:
      _rocket_handle_collision(id, &msg);
      break;
  }
}

void rocket_entity_initialize(void) {
  rocket_aux_.count = 0;
  rocket_aux_.capacity = ROCKET_AUX_CAPACITY;
  rocket_aux_.object_idx = platform_retrieve_memory(sizeof(uint32_t) * ROCKET_AUX_CAPACITY);
  rocket_aux_.smoke_model = platform_retrieve_memory(sizeof(uint16_t) * ROCKET_AUX_CAPACITY);
  rocket_aux_.fuel_ticks = platform_retrieve_memory(sizeof(uint16_t) * ROCKET_AUX_CAPACITY);
  rocket_aux_.lifetime_ticks = platform_retrieve_memory(sizeof(uint16_t) * ROCKET_AUX_CAPACITY);
  rocket_aux_.grace_ticks = platform_retrieve_memory(sizeof(uint16_t) * ROCKET_AUX_CAPACITY);
  rocket_aux_.thrust_power = platform_retrieve_memory(sizeof(float) * ROCKET_AUX_CAPACITY);

  entity_manager_vtables[ENTITY_TYPE_ROCKET].dispatch_message = _rockets_dispatch;
}

#include "platform/platform.h"
#include "platform/math.h"
#include "rocket.h"
#include "particles.h"
#include "../generated/renderer.gen.h"

#define MAX_ROCKETS 128
#define ROCKET_LIFETIME_TICKS (5 * TICKS_IN_SECOND) // 5 seconds
#define SMOKE_SPAWN_CHANCE 0.3f
#define ROCKET_MASS 0.1f
#define ROCKET_RADIUS 4.0f

// Auxiliary data for rockets (the actual physics lives in objects_data)
struct rocket_aux {
  uint32_t object_idx;      // index into objects_data
  uint16_t smoke_model;
  uint16_t lifetime_ticks;
};

static struct {
  uint32_t count;
  struct rocket_aux entries[MAX_ROCKETS];
} rocket_aux_ = { 0 };

void rocket_create(float x, float y, float vx, float vy, float ox, float oy,
                   uint16_t model_idx, uint16_t smoke_model_idx) {
  if (rocket_aux_.count >= MAX_ROCKETS) return;

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
  od->thrust[idx] = 0.0f;
  od->mass[idx] = ROCKET_MASS;
  od->parts_start_idx[idx] = 0;
  od->parts_count[idx] = 0;

  // Store auxiliary data (smoke, lifetime) - not needed by physics
  uint32_t aux_idx = rocket_aux_.count++;
  rocket_aux_.entries[aux_idx].object_idx = idx;
  rocket_aux_.entries[aux_idx].smoke_model = smoke_model_idx;
  rocket_aux_.entries[aux_idx].lifetime_ticks = ROCKET_LIFETIME_TICKS;
}

static void _rocket_spawn_smoke(struct rocket_aux* aux, struct objects_data* od) {
  if (aux->smoke_model == 0) return;
  if (randf() > SMOKE_SPAWN_CHANCE) return;

  uint32_t idx = aux->object_idx;
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
    .model_idx = aux->smoke_model
  };

  particles_create_particle(&pcm);
}

static void _rockets_tick(void) {
  struct objects_data* od = entity_manager_get_objects();

  uint32_t write_idx = 0;
  for (uint32_t i = 0; i < rocket_aux_.count; i++) {
    struct rocket_aux* aux = &rocket_aux_.entries[i];

    // spawn smoke trail
    _rocket_spawn_smoke(aux, od);

    // decrement lifetime
    if (aux->lifetime_ticks > 1) {
      aux->lifetime_ticks--;
      if (write_idx != i) {
        rocket_aux_.entries[write_idx] = *aux;
      }
      write_idx++;
    } else {
      // rocket expired - make invisible and inert (object slot remains, but it's harmless)
      uint32_t obj_idx = aux->object_idx;
      od->model_idx[obj_idx] = 0xFFFF;
      od->mass[obj_idx] = 0.0f;
      od->velocity_x[obj_idx] = 0.0f;
      od->velocity_y[obj_idx] = 0.0f;
    }
  }
  rocket_aux_.count = write_idx;
}

static void _rockets_dispatch(entity_id_t id, message_t msg) {
  (void)id;
  switch (msg.message) {
    case MESSAGE_BROADCAST_120HZ_AFTER_PHYSICS:
      _rockets_tick();
      break;
  }
}

void rocket_entity_initialize(void) {
  entity_manager_vtables[ENTITY_TYPE_ROCKET].dispatch_message = _rockets_dispatch;
}

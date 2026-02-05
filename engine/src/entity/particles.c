#include "platform/platform.h"
#include "controller.h"
#include "particles.h"
#include "fracture.h"
#include "messaging/messaging.h"
#include "debug/profiler.h"

#include "../generated/renderer.gen.h" // todo remove

static void _spawn_particle(particle_create_t* pcm) {
  struct particles_data* pd = entity_manager_get_particles();

  _ASSERT(pd->active < pd->capacity);

  uint32_t idx = pd->active;
  pd->position_orientation.position_x[idx] = pcm->x;
  pd->position_orientation.position_y[idx] = pcm->y;

  // Fragments use a small default radius (they're small debris)
  if (pcm->model_idx >= FRAGMENT_MODEL_BASE) {
    pd->position_orientation.radius[idx] = 8.0f;
  } else {
    pd->position_orientation.radius[idx] = _generated_get_model_radius(pcm->model_idx);
  }

  pd->velocity_x[idx] = pcm->vx;
  pd->velocity_y[idx] = pcm->vy;

  pd->position_orientation.orientation_x[idx] = pcm->ox;
  pd->position_orientation.orientation_y[idx] = pcm->oy;

  vec2_normalize_i(&pd->position_orientation.orientation_x[idx], &pd->position_orientation.orientation_y[idx], 1);

  pd->model_idx[idx] = pcm->model_idx;
  pd->lifetime_ticks[idx] =
    pd->lifetime_max[idx] =
    pcm->ttl;

  pd->active++;
}

void particles_create_particle(particle_create_t* pc) {
  _spawn_particle(pc);
}

static void _particles_dispatch(entity_id_t id, message_t msg) {
  (void)id;


  switch (msg.message) {
  case MESSAGE_COLLIDE_OBJECT_PARTICLE: {
    uint32_t particle_idx = (uint32_t)(msg.data_b);
    struct particles_data* pd = entity_manager_get_particles();
    if (particle_idx < pd->active) {
      pd->lifetime_ticks[particle_idx] = 0; // kill particle
    }
  } break;
  }
}

static void _move_particle(struct particles_data* pd, size_t target, size_t source) {
  pd->position_orientation.position_x[target] = pd->position_orientation.position_x[source];
  pd->position_orientation.position_y[target] = pd->position_orientation.position_y[source];
  pd->position_orientation.orientation_x[target] = pd->position_orientation.orientation_x[source];
  pd->position_orientation.orientation_y[target] = pd->position_orientation.orientation_y[source];
  pd->position_orientation.radius[target] = pd->position_orientation.radius[source];

  pd->velocity_x[target] = pd->velocity_x[source];
  pd->velocity_y[target] = pd->velocity_y[source];
  pd->lifetime_ticks[target] = pd->lifetime_ticks[source];
  pd->lifetime_max[target] = pd->lifetime_max[source];
  pd->model_idx[target] = pd->model_idx[source];

  pd->lifetime_ticks[source] = 0;
}

// Helper to free fragment slot if particle was using one
static void _free_particle_fragment(struct particles_data* pd, size_t idx) {
  uint16_t model = pd->model_idx[idx];
  if (model >= FRAGMENT_MODEL_BASE) {
    fragment_pool_free(model - FRAGMENT_MODEL_BASE);
  }
}

void entity_manager_pack_particles(void) {
  PROFILE_ZONE("entity_manager_pack_particles");
  struct particles_data* pd = entity_manager_get_particles();

  int32_t last_alive = (int32_t)(pd->active - 1);

  for (int32_t i = 0; i < last_alive; ++i) {
    if (pd->lifetime_ticks[i] == 0) {
      // Free fragment slot for this dead particle
      _free_particle_fragment(pd, i);

      for (; last_alive >= i; --last_alive) {
        if (pd->lifetime_ticks[last_alive] > 0) {
          break;
        }
        // Free fragment slots for dead particles at the end
        _free_particle_fragment(pd, last_alive);
      }

      if (i < last_alive) {
        _move_particle(pd, i, last_alive);
        --last_alive; // Consumed this particle, move to next
      } else {
        break;
      }
    }
  }

  // Handle the case where last particle(s) are dead
  for (int32_t i = last_alive; i >= 0 && pd->lifetime_ticks[i] == 0; --i) {
    _free_particle_fragment(pd, i);
    last_alive = i - 1;
  }

  pd->active = last_alive + 1;
  PROFILE_ZONE_END();
}

void particles_entity_initialize(void) {
  // we accept only broadcast for controller, no instances
  entity_manager_vtables[ENTITY_TYPE_PARTICLES].dispatch_message = _particles_dispatch;
}

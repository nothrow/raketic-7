#include "platform/platform.h"
#include "controller.h"

#include "particles.h"

static void _spawn_particle(particle_create_t* pcm) {
  struct particles_data* pd = entity_manager_get_particles();

  _ASSERT(pd->active < pd->capacity);

  uint32_t idx = pd->active;
  pd->position_orientation.position_x[idx] = pcm->x;
  pd->position_orientation.position_y[idx] = pcm->y;


  pd->velocity_x[idx] = pcm->vx;
  pd->velocity_y[idx] = pcm->vy;

  pd->position_orientation.orientation_x[idx] = pcm->vx;
  pd->position_orientation.orientation_y[idx] = pcm->vy;

  vec2_normalize_i(&pd->position_orientation.orientation_x[idx], &pd->position_orientation.orientation_y[idx], 1);

  pd->model_idx[idx] = pcm->model_idx;
  pd->lifetime_ticks[idx] = pcm->ttl;

  pd->active++;
}

void particles_create_particle(particle_create_t* pc)
{
  _spawn_particle(pc);
}

static void _particles_dispatch(entity_id_t id, message_t msg) {
  (void)id;

  switch (msg.message) {
  case MESSAGE_PARTICLE_SPAWN:
    _ASSERT(0 && "not yet implemented");
    break;
  }
}

void particles_entity_initialize(void) {
  // we accept only broadcast for controller, no instances
  entity_manager_vtables[ENTITY_TYPE_PARTICLES].dispatch_message = _particles_dispatch;
}

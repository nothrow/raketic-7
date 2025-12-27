#include "entity.h"
#include "platform/platform.h"

#define MAXSIZE 65535
#define NONEXISTENT ((size_t)(-1))

typedef struct {
  struct objects_data objects;
  struct particles_data particles;
} entity_manager_t;

static entity_manager_t manager_ = {0};

static void _objects_data_initialize(struct objects_data* data) {
  data->position = platform_retrieve_memory(sizeof(vec2_t) * MAXSIZE);
  data->velocity = platform_retrieve_memory(sizeof(vec2_t) * MAXSIZE);
  data->thrust = platform_retrieve_memory(sizeof(double) * MAXSIZE);
  data->model_idx = platform_retrieve_memory(sizeof(uint16_t) * MAXSIZE);
  data->orientation = platform_retrieve_memory(sizeof(vec2_t) * MAXSIZE);
  data->mass = platform_retrieve_memory(sizeof(double) * MAXSIZE);
  data->radius = platform_retrieve_memory(sizeof(double) * MAXSIZE);

  data->active = 0;
  data->capacity = MAXSIZE;
}

static void _particles_data_initialize(struct particles_data* data) {
  data->position = platform_retrieve_memory(sizeof(vec2_t) * MAXSIZE);
  data->velocity = platform_retrieve_memory(sizeof(vec2_t) * MAXSIZE);
  data->lifetime_ticks = platform_retrieve_memory(sizeof(uint16_t) * MAXSIZE);
  data->lifetime_max = platform_retrieve_memory(sizeof(uint16_t) * MAXSIZE);
  data->model_idx = platform_retrieve_memory(sizeof(uint16_t) * MAXSIZE);
  data->orientation = platform_retrieve_memory(sizeof(vec2_t) * MAXSIZE);
  data->temporary = platform_retrieve_memory(sizeof(struct _128bytes) * MAXSIZE);

  data->active = 0;
  data->capacity = MAXSIZE;
}

void _entity_manager_initialize(void) {
  _objects_data_initialize(&manager_.objects);
  _particles_data_initialize(&manager_.particles);

  // will be deleted - just for testing
  vec2_t center = {400.0, 300.0};

  for (int i = 0; i < 10; i++) {
    manager_.particles.active++;
    manager_.particles.position[i] = vec2_add(vec2_multiply(vec2_random(), 100), center);
    manager_.particles.velocity[i] = vec2_multiply(vec2_random(), 3);
    manager_.particles.orientation[i] = vec2_random();

    manager_.particles.lifetime_ticks[i] = manager_.particles.lifetime_max[i] = (uint16_t)(3 * TICKS_IN_SECOND);

    manager_.particles.model_idx[i] = 0;
  }

  vec2_normalize_i(manager_.particles.orientation, manager_.particles.active);
}

struct particles_data* entity_manager_get_particles(void) {
  return &manager_.particles;
}

static void _move_particle(struct particles_data* pd, size_t target, size_t source) {
  pd->position[target] = pd->position[source];
  pd->velocity[target] = pd->velocity[source];
  pd->lifetime_ticks[target] = pd->lifetime_ticks[source];
  pd->lifetime_max[target] = pd->lifetime_max[source];
  pd->model_idx[target] = pd->model_idx[source];
  pd->orientation[target] = pd->orientation[source];
}

void entity_manager_pack_particles(void) {
  struct particles_data* pd = &manager_.particles;

  int32_t last_alive = (int32_t)(pd->active - 1);

  for (int32_t i = 0; i < last_alive; ++i) {
    if (pd->lifetime_ticks[i] == 0) {
      for (; last_alive >= i; --last_alive) {
        if (pd->lifetime_ticks[last_alive] > 0) {
          break;
        }
      }

      if (i < last_alive) {
        _move_particle(pd, i, last_alive);
      } else {
        break;
      }
    }
  }
  pd->active = last_alive + 1;
}

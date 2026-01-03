#include "entity.h"
#include "platform/platform.h"
#include "../generated/renderer.gen.h"

#include "ship.h"
#include "controller.h"

#define MAXSIZE 65535
#define NONEXISTENT ((size_t)(-1))

typedef struct {
  struct objects_data objects;
  struct particles_data particles;
} entity_manager_t;


static inline entity_id_t ID_WITH_TYPE(uint32_t id, uint8_t type) {
  _ASSERT((id) <= 0x00FFFFFF);
  _ASSERT((type) <= 0xFF);

  entity_id_t ret = { ._ = ((((type) & 0xFF) << 24) | ((id) & 0x00FFFFFF)) };
  return ret;
}

object_vtable_t entity_manager_vtables[ENTITY_TYPE_COUNT] = { 0 };

int rand32();

static entity_manager_t manager_ = { 0 };

static void _position_orientation_initialize(position_orientation_t* position_orientation) {
  position_orientation->position_x = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  position_orientation->position_y = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  position_orientation->orientation_x = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  position_orientation->orientation_y = platform_retrieve_memory(sizeof(float) * MAXSIZE);
}

static void _objects_data_initialize(struct objects_data* data) {
  _position_orientation_initialize(&data->position_orientation);
  data->velocity_x = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  data->velocity_y = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  data->thrust = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  data->model_idx = platform_retrieve_memory(sizeof(uint16_t) * MAXSIZE);
  data->mass = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  data->radius = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  data->type = platform_retrieve_memory(sizeof(entity_type_t) * MAXSIZE);

  data->active = 0;
  data->capacity = MAXSIZE;
}

static void _particles_data_initialize(struct particles_data* data) {
  _position_orientation_initialize(&data->position_orientation);

  data->velocity_x = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  data->velocity_y = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  data->lifetime_ticks = platform_retrieve_memory(sizeof(uint16_t) * MAXSIZE);
  data->lifetime_max = platform_retrieve_memory(sizeof(uint16_t) * MAXSIZE);
  data->model_idx = platform_retrieve_memory(sizeof(uint16_t) * MAXSIZE);
  data->temporary = platform_retrieve_memory(sizeof(struct _128bytes) * MAXSIZE);

  data->active = 0;
  data->capacity = MAXSIZE;
}

static void _generate_dummy_data(void) {
  // will be deleted - just for testing
  manager_.objects.active = 1;
  manager_.objects.position_orientation.position_x[0] = 400.0f;
  manager_.objects.position_orientation.position_y[0] = 300.0f;
  manager_.objects.velocity_x[0] = 0.0f;
  manager_.objects.velocity_y[0] = 0.1f;
  manager_.objects.position_orientation.orientation_x[0] = 0.3f;
  manager_.objects.position_orientation.orientation_y[0] = -0.8f;
  manager_.objects.model_idx[0] = MODEL_SHIP_IDX;
  manager_.objects.type[0]._ = ENTITY_TYPE_SHIP;
  manager_.objects.mass[0] = 1000.0f;
  manager_.objects.radius[0] = 20.0f;

  vec2_normalize_i(manager_.objects.position_orientation.orientation_x,
                   manager_.objects.position_orientation.orientation_y, manager_.objects.active);

  for (int i = 0; i < 10; i++) {
    manager_.particles.active++;
    manager_.particles.position_orientation.position_x[i] =
        ((float)(rand32() % 1000) / 1000.0f * 2.0f - 1.0f) * 100.0f + 400.0f;
    manager_.particles.position_orientation.position_y[i] =
        ((float)(rand32() % 1000) / 1000.0f * 2.0f - 1.0f) * 100.0f + 300.0f;
    manager_.particles.velocity_x[i] = ((float)(rand32() % 1000) / 1000.0f * 2.0f - 1.0f) * 3.0f;
    manager_.particles.velocity_y[i] = ((float)(rand32() % 1000) / 1000.0f * 2.0f - 1.0f) * 3.0f;
    manager_.particles.position_orientation.orientation_x[i] = (float)(rand32() % 1000) / 1000.0f * 2.0f - 1.0f;
    manager_.particles.position_orientation.orientation_y[i] = (float)(rand32() % 1000) / 1000.0f * 2.0f - 1.0f;

    manager_.particles.lifetime_ticks[i] = manager_.particles.lifetime_max[i] = (uint16_t)(3 * TICKS_IN_SECOND);

    manager_.particles.model_idx[i] = 0;
  }

  vec2_normalize_i(manager_.particles.position_orientation.orientation_x,
                   manager_.particles.position_orientation.orientation_y, manager_.particles.active);

  controller_set_entity(ID_WITH_TYPE(0, ENTITY_TYPE_SHIP));
}

void entity_manager_initialize(void) {
  _objects_data_initialize(&manager_.objects);
  _particles_data_initialize(&manager_.particles);

  ship_entity_initialize();
  controller_entity_initialize();

  _generate_dummy_data();
}

struct particles_data* entity_manager_get_particles(void) {
  return &manager_.particles;
}

struct objects_data* entity_manager_get_objects(void) {
  return &manager_.objects;
}

static void _move_particle(struct particles_data* pd, size_t target, size_t source) {
  pd->position_orientation.position_x[target] = pd->position_orientation.position_x[source];
  pd->position_orientation.position_y[target] = pd->position_orientation.position_y[source];
  pd->velocity_x[target] = pd->velocity_x[source];
  pd->velocity_y[target] = pd->velocity_y[source];
  pd->lifetime_ticks[target] = pd->lifetime_ticks[source];
  pd->lifetime_max[target] = pd->lifetime_max[source];
  pd->model_idx[target] = pd->model_idx[source];
  pd->position_orientation.orientation_x[target] = pd->position_orientation.orientation_x[source];
  pd->position_orientation.orientation_y[target] = pd->position_orientation.orientation_y[source];
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

void entity_manager_dispatch_message(entity_id_t recipient_id, message_t msg) {
  uint8_t entity_type = GET_TYPE(recipient_id);

  if (recipient_id._ == RECIPIENT_ID_BROADCAST._) {
    for (size_t i = 1; i < ENTITY_TYPE_COUNT; i++) {
      entity_manager_vtables[i].dispatch_message(recipient_id, msg);
    }
  } else if (entity_type != RECIPIENT_TYPE_ANY._) {
    _ASSERT(entity_type < ENTITY_TYPE_COUNT);
    entity_manager_vtables[entity_type].dispatch_message(recipient_id, msg);
  } else {
    _ASSERT(0 && "missing type in id");
  }
}

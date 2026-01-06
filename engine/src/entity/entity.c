#include "entity.h"
#include "platform/platform.h"
#include "platform/math.h"
#include "../generated/renderer.gen.h"

#include "ship.h"
#include "controller.h"
#include "engine.h"
#include "particles.h"
#include "debug/debug.h"

#define MAXSIZE 65535
#define NONEXISTENT ((size_t)(-1))

typedef struct {
  struct objects_data objects;
  struct particles_data particles;
  struct parts_data parts;
} entity_manager_t;

object_vtable_t entity_manager_vtables[ENTITY_TYPE_COUNT] = { 0 };

static entity_manager_t manager_ = { 0 };

static void _position_orientation_initialize(position_orientation_t* position_orientation) {
  position_orientation->position_x = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  position_orientation->position_y = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  position_orientation->orientation_x = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  position_orientation->orientation_y = platform_retrieve_memory(sizeof(float) * MAXSIZE);
}

static void _parts_data_initialize(struct parts_data* data) {
  _position_orientation_initialize(&data->world_position_orientation);
  data->parent_id = platform_retrieve_memory(sizeof(entity_id_t) * MAXSIZE);
  data->type = platform_retrieve_memory(sizeof(entity_type_t) * MAXSIZE);
  data->local_offset_x = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  data->local_offset_y = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  data->local_orientation_x = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  data->local_orientation_y = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  data->model_idx = platform_retrieve_memory(sizeof(uint16_t) * MAXSIZE);
  data->data = platform_retrieve_memory(sizeof(struct _128bytes) * MAXSIZE);
  data->active = 0;
  data->capacity = MAXSIZE;
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
  data->parts_start_idx = platform_retrieve_memory(sizeof(uint32_t) * MAXSIZE);
  data->parts_count = platform_retrieve_memory(sizeof(uint32_t) * MAXSIZE);

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
  entity_id_t player = _generate_entity_by_model(ENTITY_TYPEREF_SHIP, MODEL_SHIP_IDX);

  debug_watch_set(player);
  controller_set_entity(player);
}

static void _entity_manager_types_initialize(void) {
  particles_entity_initialize();
  ship_entity_initialize();
  controller_entity_initialize();
  engine_part_entity_initialize();
}

void entity_manager_initialize(void) {
  _objects_data_initialize(&manager_.objects);
  _particles_data_initialize(&manager_.particles);
  _parts_data_initialize(&manager_.parts);

  _entity_manager_types_initialize();

  _generate_dummy_data();
}

struct particles_data* entity_manager_get_particles(void) {
  return &manager_.particles;
}

struct objects_data* entity_manager_get_objects(void) {
  return &manager_.objects;
}

struct parts_data* entity_manager_get_parts(void) {
  return &manager_.parts;
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
    _ASSERT(entity_type >= 0 && entity_type < ENTITY_TYPE_COUNT);

    entity_manager_vtables[entity_type].dispatch_message(recipient_id, msg);
  } else {
    _ASSERT(0 && "missing type in id");
  }
}

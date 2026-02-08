#include "entity.h"
#include "platform/platform.h"
#include "platform/math.h"
#include "../generated/renderer.gen.h"

#include "ship.h"
#include "autopilot.h"
#include "controller.h"
#include "engine.h"
#include "weapon.h"
#include "rocket.h"
#include "beams.h"
#include "particles.h"
#include "asteroid.h"

#include "camera.h"
#include "planet.h"
#include "moon.h"
#include "debug/debug.h"
#include "debug/profiler.h"

#define MAXSIZE 65535
#define NONEXISTENT ((size_t)(-1))

typedef struct {
  struct objects_data objects;
  struct particles_data particles;
  struct parts_data parts;
  uint32_t* object_remap; // remap table for object compaction
} entity_manager_t;

object_vtable_t entity_manager_vtables[ENTITY_TYPE_COUNT] = { 0 };

static entity_manager_t manager_ = { 0 };

static void _position_orientation_initialize(position_orientation_t* position_orientation) {
  position_orientation->position_x = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  position_orientation->position_y = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  position_orientation->orientation_x = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  position_orientation->orientation_y = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  position_orientation->radius = platform_retrieve_memory(sizeof(float) * MAXSIZE);
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

  data->acceleration_x = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  data->acceleration_y = platform_retrieve_memory(sizeof(float) * MAXSIZE);

  data->thrust = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  data->model_idx = platform_retrieve_memory(sizeof(uint16_t) * MAXSIZE);
  data->mass = platform_retrieve_memory(sizeof(float) * MAXSIZE);
  data->type = platform_retrieve_memory(sizeof(entity_type_t) * MAXSIZE);
  data->parts_start_idx = platform_retrieve_memory(sizeof(uint32_t) * MAXSIZE);
  data->parts_count = platform_retrieve_memory(sizeof(uint32_t) * MAXSIZE);
  data->health = platform_retrieve_memory(sizeof(int16_t) * MAXSIZE);

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

  platform_clear_memory(data->lifetime_max, sizeof(uint16_t) * MAXSIZE);
  platform_clear_memory(data->lifetime_ticks, sizeof(uint16_t) * MAXSIZE);

  data->active = 0;
  data->capacity = MAXSIZE;
}

static void _entity_manager_types_initialize(void) {
  particles_entity_initialize();
  ship_entity_initialize();
  autopilot_initialize();
  controller_entity_initialize();
  engine_part_entity_initialize();
  weapon_part_entity_initialize();
  rocket_entity_initialize();
  beams_entity_initialize();
  camera_entity_initialize();
  planet_entity_initialize();
  asteroid_entity_initialize();
  chunk_part_entity_initialize();
  moon_entity_initialize();
}

void entity_manager_initialize(void) {
  _objects_data_initialize(&manager_.objects);
  _particles_data_initialize(&manager_.particles);
  _parts_data_initialize(&manager_.parts);
  manager_.object_remap = platform_retrieve_memory(sizeof(uint32_t) * MAXSIZE);

  _entity_manager_types_initialize();

#ifndef UNIT_TESTS
  _generated_load_map_data(0);
#endif
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

void entity_manager_get_vectors(entity_id_t entity_id, float* pos, float* vel) {
  struct objects_data* od = &manager_.objects;
  size_t idx = GET_ORDINAL(entity_id);
  if (pos != NULL) {
    pos[0] = od->position_orientation.position_x[idx];
    pos[1] = od->position_orientation.position_y[idx];
  }

  if (vel != NULL) {
    vel[0] = od->velocity_x[idx];
    vel[1] = od->velocity_y[idx];
  }
}

entity_id_t entity_manager_resolve_object(uint32_t ordinal) {
  struct objects_data* od = &manager_.objects;
  _ASSERT(ordinal < od->active);

  entity_type_t type = od->type[ordinal];
  entity_id_t ret = OBJECT_ID_WITH_TYPE(ordinal, type._);
  return ret;
}

entity_id_t entity_manager_resolve_part(uint32_t ordinal) {
  struct parts_data* pd = &manager_.parts;
  _ASSERT(ordinal < pd->active);

  entity_type_t type = pd->type[ordinal];
  entity_id_t ret = PART_ID_WITH_TYPE(ordinal, type._);
  return ret;
}

static void _move_object(struct objects_data* od, uint32_t target, uint32_t source) {
  od->position_orientation.position_x[target] = od->position_orientation.position_x[source];
  od->position_orientation.position_y[target] = od->position_orientation.position_y[source];
  od->position_orientation.orientation_x[target] = od->position_orientation.orientation_x[source];
  od->position_orientation.orientation_y[target] = od->position_orientation.orientation_y[source];
  od->position_orientation.radius[target] = od->position_orientation.radius[source];

  od->velocity_x[target] = od->velocity_x[source];
  od->velocity_y[target] = od->velocity_y[source];
  od->acceleration_x[target] = od->acceleration_x[source];
  od->acceleration_y[target] = od->acceleration_y[source];
  od->thrust[target] = od->thrust[source];
  od->mass[target] = od->mass[source];
  od->type[target] = od->type[source];
  od->model_idx[target] = od->model_idx[source];
  od->parts_start_idx[target] = od->parts_start_idx[source];
  od->parts_count[target] = od->parts_count[source];
  od->health[target] = od->health[source];
}

void entity_manager_pack_objects(void) {
  struct objects_data* od = &manager_.objects;

  // Check if any objects are dead
  bool has_dead = false;
  for (uint32_t i = 0; i < od->active; i++) {
    if (od->health[i] <= 0) {
      has_dead = true;
      break;
    }
  }
  if (!has_dead) return;

  PROFILE_ZONE("entity_manager_pack_objects");

  uint32_t* remap = manager_.object_remap;
  uint32_t old_active = od->active;

  // Initialize remap to identity
  for (uint32_t i = 0; i < old_active; i++) {
    remap[i] = i;
  }

  // Compact: swap dead with alive from end (same pattern as pack_particles)
  int32_t last_alive = (int32_t)(od->active - 1);

  for (int32_t i = 0; i < last_alive; ++i) {
    if (od->health[i] <= 0) {
      // Find last alive from end
      for (; last_alive > i; --last_alive) {
        if (od->health[last_alive] > 0) {
          break;
        }
      }

      if (i < last_alive) {
        // Swap: move last_alive to position i
        remap[last_alive] = (uint32_t)i;
        remap[i] = UINT32_MAX; // dead, mark invalid

        _move_object(od, (uint32_t)i, (uint32_t)last_alive);
        od->health[last_alive] = 0; // mark source as dead
        --last_alive;
      } else {
        remap[i] = UINT32_MAX;
        break;
      }
    }
  }

  // Handle trailing dead objects
  for (int32_t i = last_alive; i >= 0 && od->health[i] <= 0; --i) {
    remap[i] = UINT32_MAX;
    last_alive = i - 1;
  }

  od->active = (uint32_t)(last_alive + 1);

  // Remap all references
  // 1. Parts: remap parent_id, kill orphaned blocks, compact, rebuild parts_start_idx
  {
    struct parts_data* pd = &manager_.parts;

    // 1a. Per block: remap live parents, mark dead blocks
    for (uint32_t i = 0; i < pd->active; i += 8) {
      if (pd->model_idx[i] == 0xFFFF) continue; // padding-only block

      uint32_t old_ord = GET_ORDINAL(pd->parent_id[i]);
      if (old_ord >= old_active) continue;

      uint32_t new_ord = remap[old_ord];
      if (new_ord != UINT32_MAX) {
        // Parent survived -- remap parent_id for all real parts in block
        uint8_t type = GET_TYPE(pd->parent_id[i]);
        entity_id_t new_id = OBJECT_ID_WITH_TYPE(new_ord, type);
        for (uint32_t k = 0; k < 8; k++) {
          if (pd->model_idx[i + k] != 0xFFFF)
            pd->parent_id[i + k] = new_id;
        }
      } else {
        // Parent is dead -- mark entire block dead
        for (uint32_t k = 0; k < 8; k++)
          pd->model_idx[i + k] = 0xFFFF;
      }
    }

    // 1b. Compact live blocks forward (preserves 8-alignment for SIMD physics)
    //     and rebuild every live object's parts_start_idx
    uint32_t write = 0;
    uint32_t last_parent = UINT32_MAX;
    for (uint32_t i = 0; i < pd->active; i += 8) {
      if (pd->model_idx[i] == 0xFFFF) continue; // dead block, skip

      uint32_t parent_idx = GET_ORDINAL(pd->parent_id[i]);

      // Set parts_start_idx on first block of each parent
      if (parent_idx != last_parent) {
        od->parts_start_idx[parent_idx] = write;
        last_parent = parent_idx;
      }

      if (write != i) {
        for (uint32_t k = 0; k < 8; k++) {
          pd->parent_id[write + k] = pd->parent_id[i + k];
          pd->type[write + k] = pd->type[i + k];
          pd->local_offset_x[write + k] = pd->local_offset_x[i + k];
          pd->local_offset_y[write + k] = pd->local_offset_y[i + k];
          pd->local_orientation_x[write + k] = pd->local_orientation_x[i + k];
          pd->local_orientation_y[write + k] = pd->local_orientation_y[i + k];
          pd->model_idx[write + k] = pd->model_idx[i + k];
          pd->data[write + k] = pd->data[i + k];
        }
      }
      write += 8;
    }
    pd->active = write;
  }

  // 2. Controller, Camera, Rocket aux
  controller_remap_entity(remap, old_active);
  camera_remap_entity(remap, old_active);
  rocket_remap_objects(remap, old_active);
  autopilot_remap(remap, old_active);

  PROFILE_ZONE_END();
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

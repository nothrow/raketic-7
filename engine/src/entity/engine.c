#include "platform/platform.h"
#include "engine.h"

#include "particles.h"
#include "../generated/renderer.gen.h"

struct engine_data {
  float thrust;
};

static void _set_part_thrust(uint32_t idx, float percentage) {
  struct parts_data* pd = entity_manager_get_parts();

  struct engine_data* ed = (struct engine_data*)(pd->data[idx].data);
  ed->thrust = percentage;
}

static void _engine_set_thrust_percentage(entity_id_t id, float percentage) {
  _ASSERT(is_valid_id(id));

  if (IS_PART(id)) {
    _set_part_thrust(GET_ORDINAL(id), percentage);
  } else {
    uint32_t object_idx = GET_ORDINAL(id);

    struct objects_data* od = entity_manager_get_objects();
    struct parts_data* pd = entity_manager_get_parts();


    for (uint32_t i = od->parts_start_idx[object_idx];
         i < od->parts_start_idx[object_idx] + od->parts_count[object_idx]; ++i) {
      if (pd->type[i]._ == ENTITY_TYPE_PART_ENGINE) {
        _set_part_thrust(i, percentage);
      }
    }
  }
}

static void _engine_tick() {
  struct parts_data* pd = entity_manager_get_parts();
  struct objects_data* od = entity_manager_get_objects();

  for (size_t i = 0; i < pd->active; i++) {
    if (pd->type[i]._ == ENTITY_TYPE_PART_ENGINE) {
      struct engine_data* ed = (struct engine_data*)(pd->data[i].data);

      if (ed->thrust > 0.0f) {
        float ox = od->position_orientation.orientation_x[GET_ORDINAL(pd->parent_id[i])];
        float oy = od->position_orientation.orientation_y[GET_ORDINAL(pd->parent_id[i])];

        particle_create_t pcm = {
          .x = pd->world_position_orientation.position_x[i],
          .y = pd->world_position_orientation.position_y[i],
          .vx = -ox * ed->thrust * 100.0f,
          .vy = -oy * ed->thrust * 100.0f,
          .ttl = 3 * TICKS_IN_SECOND,
          .model_idx = MODEL_EXHAUST_IDX
        };

        particles_create_particle(&pcm);
      }
    }
  }
}

static void _engine_part_dispatch(entity_id_t id, message_t msg) {
  (void)id;
  switch (msg.message) {
  case MESSAGE_SHIP_ENGINES_THRUST:
    if (msg.data_a > 0) {
      // thrust on
      _engine_set_thrust_percentage(id, (float)(msg.data_a) / 100.0f);
    } else {
      // thrust off
      _engine_set_thrust_percentage(id, 0.0f);
    }
    break;
  case MESSAGE_BROADCAST_120HZ_TICK:
    _engine_tick();
  }
}

void engine_part_entity_initialize(void) {
  entity_manager_vtables[ENTITY_TYPE_PART_ENGINE].dispatch_message = _engine_part_dispatch;
}

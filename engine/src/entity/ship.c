
#include "platform/platform.h"
#include "platform/math.h"

#include "entity_internal.h"
#include "entity.h"
#include "engine.h"
#include "ship.h"

#define MAXSIZE 8192

static void _ship_rotate_by(entity_id_t idx, int32_t rotation) {
  uint32_t id = GET_ORDINAL(idx);
  struct objects_data* od = entity_manager_get_objects();

  float ox = od->position_orientation.orientation_x[id];
  float oy = od->position_orientation.orientation_y[id];

  // 2D rotation matrix: [cos -sin] [x]   [x*cos - y*sin]
  //                     [sin  cos] [y] = [x*sin + y*cos]
  float c = lut_cos(rotation);
  float s = lut_sin(rotation);

  od->position_orientation.orientation_x[id] = ox * c - oy * s;
  od->position_orientation.orientation_y[id] = ox * s + oy * c;

  // renormalize to avoid drift over many rotations
  vec2_normalize_i(&od->position_orientation.orientation_x[id], &od->position_orientation.orientation_y[id], 1);
}

static void _ship_apply_thrust_from_engines(void) {
  struct objects_data* od = entity_manager_get_objects();
  struct parts_data* pd = entity_manager_get_parts();

  for (size_t i = 0; i < od->active; i++) {
    od->thrust[i] = 0.0f;

    for (size_t j = 0; j < od->parts_count[i]; j++) {
      uint32_t part_idx = od->parts_start_idx[i] + j;
      if (pd->type[part_idx]._ != ENTITY_TYPE_PART_ENGINE) {
        continue;
      }
      struct engine_data* ed = (struct engine_data*)(pd->data[part_idx].data);
      od->thrust[i] += ed->thrust;
    }  
  }


}

static void _ship_rotate_to(entity_id_t id, float x, float y) {
  uint32_t obj_idx = GET_ORDINAL(id);
  struct objects_data* od = entity_manager_get_objects();

  od->position_orientation.orientation_x[obj_idx] = x;
  od->position_orientation.orientation_y[obj_idx] = y;

  // renormalize to avoid drift
  vec2_normalize_i(&od->position_orientation.orientation_x[obj_idx], &od->position_orientation.orientation_y[obj_idx], 1);
}

static void _ship_dispatch(entity_id_t id, message_t msg) {
  switch (msg.message) {
  case MESSAGE_SHIP_ROTATE_BY:
    _ship_rotate_by(id, msg.data_a);
    break;
  case MESSAGE_BROADCAST_120HZ_BEFORE_PHYSICS:
    _ship_apply_thrust_from_engines();
    break;
  case MESSAGE_SHIP_ROTATE_TO:
    _ship_rotate_to(id, _i2f(msg.data_a), _i2f(msg.data_b));
    break;
  }
}

void ship_entity_initialize(void) {
  entity_manager_vtables[ENTITY_TYPE_SHIP].dispatch_message = _ship_dispatch;
}

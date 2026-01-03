
#include "platform/platform.h"
#include "platform/math.h"

#include "entity_internal.h"
#include "entity.h"
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

static void _ship_dispatch(entity_id_t id, message_t msg) {
  switch (msg.message) {
  case MESSAGE_SHIP_ROTATE_BY:
    _ship_rotate_by(id, msg.data_a);
    break;
  }
}

void ship_entity_initialize(void) {
  entity_manager_vtables[ENTITY_TYPE_SHIP].dispatch_message = _ship_dispatch;
}

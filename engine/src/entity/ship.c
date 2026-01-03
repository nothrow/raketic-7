
#include "platform/platform.h"
#include "platform/math.h"

#include "entity_internal.h"
#include "entity.h"
#include "ship.h"

#define MAXSIZE 8192

typedef struct {
  uint32_t active;
  uint32_t capacity;

  entity_id_t* rawid; // index in entity manager
} ship_manager_t;


static ship_manager_t ship_manager_ = { 0 };

static entity_id_t _ship_lookup_raw(entity_id_t id) {
  _ASSERT(GET_TYPE(id) == ENTITY_TYPE_SHIP);

  uint32_t refid = GET_REFID(id);

  _ASSERT(refid < ship_manager_.active);
  return ship_manager_.rawid[refid];
}

static void _ship_rotate_by(entity_id_t idx, int32_t rotation) {
  struct objects_data* od = entity_manager_get_objects();

  float ox = od->position_orientation.orientation_x[idx];
  float oy = od->position_orientation.orientation_y[idx];

  // 2D rotation matrix: [cos -sin] [x]   [x*cos - y*sin]
  //                     [sin  cos] [y] = [x*sin + y*cos]
  float c = lut_cos(rotation);
  float s = lut_sin(rotation);

  od->position_orientation.orientation_x[idx] = ox * c - oy * s;
  od->position_orientation.orientation_y[idx] = ox * s + oy * c;

  // renormalize to avoid drift over many rotations
  vec2_normalize_i(&od->position_orientation.orientation_x[idx], &od->position_orientation.orientation_y[idx], 1);
}

static void _ship_dispatch(entity_id_t id, message_t msg) {
  switch (msg.message) {
  case MESSAGE_SHIP_ROTATE_BY:
    _ship_rotate_by(_ship_lookup_raw(id), msg.data_a);
    break;
  }
}

void ship_entity_initialize(void) {
  ship_manager_.capacity = MAXSIZE;
  ship_manager_.rawid = platform_retrieve_memory(sizeof(entity_id_t) * MAXSIZE);

  entity_manager_vtables[ENTITY_TYPE_SHIP].lookup_raw = _ship_lookup_raw;
  entity_manager_vtables[ENTITY_TYPE_SHIP].dispatch_message = _ship_dispatch;

  // will be deleted - just for testing
  ship_manager_.active = 1;
  ship_manager_.rawid[0] = 0;
}

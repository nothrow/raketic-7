#include "platform/platform.h"
#include "engine.h"

static void _engine_part_dispatch(entity_id_t id, message_t msg) {
  (void)id;
  switch (msg.message) {
  case MESSAGE_SHIP_ENGINES_THRUST: {
    /*uint32_t part_id = GET_ORDINAL(id);
    struct parts_data* pd = entity_manager_get_parts();
    entity_id_t parent_id = pd->parent_id[part_id];
    if (is_valid_id(parent_id)) {
      uint32_t object_id = GET_ORDINAL(parent_id);
      struct objects_data* od = entity_manager_get_objects();
      // set thrust
      od->thrust[object_id] = (float)(msg.data_a);
    }*/
  } break;
  }
}

void engine_part_entity_initialize(void) {
  entity_manager_vtables[ENTITY_TYPE_PART_ENGINE].dispatch_message = _engine_part_dispatch;
}

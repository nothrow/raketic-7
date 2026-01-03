#include "controller.h"

static entity_id_t _controlled_entity = 0xFFFFFFFF;
static int _last_rot = 0;

static entity_id_t _controller_lookup_raw(entity_id_t id) {
  (void)id;
  _ASSERT(id == 0 && "No controller instances exist");
  return 0;
}

static void _controller_dispatch(entity_id_t id, message_t msg) {
  (void)id;

  switch (msg.message) {
  case MESSAGE_CONTROLLER_MOUSEMOVE:
    _last_rot = msg.data_a;
    break;
  case MESSAGE_BROADCAST_TICK:
    if (_controlled_entity != 0xFFFFFFFF && _last_rot != 0) {
      messaging_send(ENTITY_TYPE_SHIP, _controlled_entity, CREATE_MESSAGE(MESSAGE_SHIP_ROTATE_BY, _last_rot, 0));
      _last_rot = 0;
    }
    break;
  }
}

void controller_set_entity(entity_id_t entity_id) {
  _controlled_entity = entity_id;
}

void controller_entity_initialize(void) {
  // we accept only broadcast for controller, no instances
  entity_manager_vtables[ENTITY_TYPE_CONTROLLER].lookup_raw = _controller_lookup_raw;
  entity_manager_vtables[ENTITY_TYPE_CONTROLLER].dispatch_message = _controller_dispatch;
}

#include "controller.h"

static entity_id_t _controller_lookup_raw(entity_id_t id) {
  (void)id;
  _ASSERT(id == 0 && "No controller instances exist");
  return 0;
}

static void _controller_dispatch(entity_id_t id, message_t msg) {
  // for now, no messages handled
  (void)id;
  (void)msg;
}

void controller_entity_initialize(void)
{
  // we accept only broadcast for controller, no instances
  entity_manager_vtables[ENTITY_TYPE_CONTROLLER].lookup_raw = _controller_lookup_raw;
  entity_manager_vtables[ENTITY_TYPE_CONTROLLER].dispatch_message = _controller_dispatch;
}

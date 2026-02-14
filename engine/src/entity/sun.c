#include "platform/platform.h"
#include "entity_internal.h"
#include "entity.h"
#include "sun.h"

static void _sun_dispatch(entity_id_t id, message_t msg) {
  (void)id;

  switch (msg.message) {
  case MESSAGE_BROADCAST_120HZ_BEFORE_PHYSICS:
    // Future: heat damage to nearby objects
    break;
  }
}

void sun_entity_initialize(void) {
  entity_manager_vtables[ENTITY_TYPE_SUN].dispatch_message = _sun_dispatch;
}

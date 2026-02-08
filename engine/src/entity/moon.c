#include "platform/platform.h"
#include "platform/math.h"

#include "entity_internal.h"
#include "entity.h"
#include "moon.h"

static void _moon_dispatch(entity_id_t id, message_t msg) {
  (void)id;

  switch (msg.message) {
  case MESSAGE_BROADCAST_120HZ_BEFORE_PHYSICS:
    break;
  }
}

void moon_entity_initialize(void) {
  entity_manager_vtables[ENTITY_TYPE_MOON].dispatch_message = _moon_dispatch;
}

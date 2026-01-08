#include "platform/platform.h"
#include "platform/math.h"

#include "entity_internal.h"
#include "entity.h"
#include "planet.h"

static void _planet_dispatch(entity_id_t id, message_t msg) {
  (void)id;

  switch (msg.message) {
  case MESSAGE_BROADCAST_120HZ_BEFORE_PHYSICS:
    // Future: planet rotation update
    break;
  }
}

void planet_entity_initialize(void) {
  entity_manager_vtables[ENTITY_TYPE_PLANET].dispatch_message = _planet_dispatch;
}

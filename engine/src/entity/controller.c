#include "platform/platform.h"
#include "controller.h"

static entity_id_t _controlled_entity = 0xFFFFFFFF;
static int _last_rot = 0;

static entity_id_t _controller_lookup_raw(entity_id_t id) {
  (void)id;
  _ASSERT(id == 0 && "No controller instances exist");
  return 0;
}

static void _process_mouse() {
  const struct input_state * input = platform_get_input_state();

  if (input->mdx != 0) {
    messaging_send(ENTITY_TYPE_SHIP, _controlled_entity, CREATE_MESSAGE(MESSAGE_SHIP_ROTATE_BY, -input->mdx, 0));
  }

  if (platform_input_is_button_down(BUTTON_LEFT)) {
    messaging_send(ENTITY_TYPE_SHIP, _controlled_entity, CREATE_MESSAGE(MESSAGE_SHIP_ENGINES_THRUST, 100, 0));
    // thrust on
  } else {
    messaging_send(ENTITY_TYPE_SHIP, _controlled_entity, CREATE_MESSAGE(MESSAGE_SHIP_ENGINES_THRUST, 0, 0));
    // thrust off
  }
}

static void _controller_dispatch(entity_id_t id, message_t msg) {
  (void)id;

  switch (msg.message) {
  case MESSAGE_BROADCAST_FRAME_TICK:
    if (_controlled_entity != 0xFFFFFFFF) {
      _process_mouse();
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

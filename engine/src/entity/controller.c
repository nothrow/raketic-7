#include "platform/platform.h"
#include "controller.h"

static entity_id_t _controlled_entity = INVALID_ENTITY;
static int _last_rot = 0;
static bool _autopilot_active = false;

static void _process_mouse() {
  const struct input_state* input = platform_get_input_state();

  if (input->mdx != 0) {
    messaging_send(_controlled_entity, CREATE_MESSAGE(MESSAGE_SHIP_ROTATE_BY, -input->mdx, 0));
  }

  if (platform_input_is_button_down(BUTTON_RIGHT)) {
    messaging_send(PARTS_OF_TYPE(_controlled_entity, PART_TYPEREF_ENGINE),
                   CREATE_MESSAGE(MESSAGE_SHIP_ENGINES_THRUST, 100, 0));
    // thrust on
  } else {
    messaging_send(PARTS_OF_TYPE(_controlled_entity, PART_TYPEREF_ENGINE),
                   CREATE_MESSAGE(MESSAGE_SHIP_ENGINES_THRUST, 0, 0));
    // thrust off
  }
  
  // Fire weapons with left mouse button
  if (platform_input_is_button_down(BUTTON_LEFT)) {
    messaging_send(PARTS_OF_TYPE(_controlled_entity, PART_TYPEREF_WEAPON),
                   CREATE_MESSAGE(MESSAGE_WEAPON_FIRE, 1, 0));
  } else {
    messaging_send(PARTS_OF_TYPE(_controlled_entity, PART_TYPEREF_WEAPON),
                   CREATE_MESSAGE(MESSAGE_WEAPON_FIRE, 0, 0));
  }
}

static void _process_keyboard() {
  if (platform_input_is_key_down(KEY_SPACE)) {

    // reverse
    float velocity[2];
    entity_manager_get_vectors(_controlled_entity, NULL, velocity);

    messaging_send(_controlled_entity, CREATE_MESSAGE(MESSAGE_SHIP_ROTATE_TO, _f2i(-velocity[0]), _f2i(-velocity[1])));
  }
}

static void _process_autopilot_cancel() {
  // Cancel autopilot on thrust or SPACE
  if (platform_input_is_button_down(BUTTON_RIGHT) || platform_input_is_key_down(KEY_SPACE)) {
    _autopilot_active = false;
    messaging_send(_controlled_entity, CREATE_MESSAGE(MESSAGE_SHIP_AUTOPILOT_DISENGAGE, 0, 0));
  }
}

static void _controller_dispatch(entity_id_t id, message_t msg) {
  (void)id;

  switch (msg.message) {
  case MESSAGE_BROADCAST_FRAME_TICK:
    if (is_valid_id(_controlled_entity)) {
      if (_autopilot_active) {
        _process_autopilot_cancel();
      } else {
        _process_mouse();
        _process_keyboard();
      }
    }
    break;
  case MESSAGE_SHIP_AUTOPILOT_ENGAGE:
    _autopilot_active = true;
    break;
  case MESSAGE_SHIP_AUTOPILOT_DISENGAGE:
    _autopilot_active = false;
    break;
  }
}

void controller_set_entity(entity_id_t entity_id) {
  _controlled_entity = entity_id;
}

void controller_remap_entity(uint32_t* remap, uint32_t old_active) {
  if (is_valid_id(_controlled_entity)) {
    uint32_t old_ord = GET_ORDINAL(_controlled_entity);
    if (old_ord < old_active) {
      uint32_t new_ord = remap[old_ord];
      if (new_ord != UINT32_MAX) {
        uint8_t type = GET_TYPE(_controlled_entity);
        _controlled_entity = OBJECT_ID_WITH_TYPE(new_ord, type);
      } else {
        _controlled_entity = (entity_id_t)INVALID_ENTITY;
      }
    }
  }
}

void controller_entity_initialize(void) {
  // we accept only broadcast for controller, no instances
  entity_manager_vtables[ENTITY_TYPE_CONTROLLER].dispatch_message = _controller_dispatch;
}

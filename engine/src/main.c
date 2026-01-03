#include "platform/platform.h"
#include "entity/entity.h"
#include "physics/physics.h"
#include "messaging/messaging.h"
#include "graphics/graphics.h"

int run(void) {
  bool running = true;

  platform_initialize();
  messaging_initialize();
  entity_manager_initialize();

  messaging_send(RECIPIENT_TYPE_BROADCAST, RECIPIENT_ID_BROADCAST, CREATE_MESSAGE(MESSAGE_BROADCAST_SYSTEM_INITIALIZED, 0, 0));

  while (running) {
    platform_frame_start();
    running = platform_loop();

    if (!running) // fail fast
      break;

    while (platform_tick_pending()) {
      // update game
      physics_engine_tick();
      messaging_pump();
    }

    graphics_engine_draw();

    platform_frame_end();
  }
  return 0;
}

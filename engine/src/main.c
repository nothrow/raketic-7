#include "platform/platform.h"
#include "entity/entity.h"
#include "physics/physics.h"
#include "messaging/messaging.h"
#include "graphics/graphics.h"
#include "debug/debug.h"

int run(void) {
  bool running = true;

  debug_initialize();
  platform_initialize();
  messaging_initialize();
  entity_manager_initialize();
  graphics_initialize();


  messaging_send(RECIPIENT_ID_BROADCAST, CREATE_MESSAGE(MESSAGE_BROADCAST_SYSTEM_INITIALIZED, 0, 0));

  while (running) {
    platform_frame_start();
    running = platform_loop();

    if (!running) // fail fast
      break;

    messaging_send(RECIPIENT_ID_BROADCAST, CREATE_MESSAGE(MESSAGE_BROADCAST_FRAME_TICK, 0, 0));

    while (platform_tick_pending()) {
      // update game
      physics_engine_tick();
      messaging_pump();
    }

    // TODO: Replace with actual camera position when implemented
    graphics_engine_draw(0, 0);
    debug_watch_draw();

    platform_frame_end();
  }
  return 0;
}

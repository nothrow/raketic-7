#include "platform/platform.h"
#include "entity/entity.h"
#include "entity/fracture.h"
#include "collisions/collisions.h"
#include "physics/physics.h"
#include "messaging/messaging.h"
#include "graphics/graphics.h"
#include "debug/debug.h"
#include "debug/profiler.h"

// TEST: Explode ship periodically (every 3 seconds)
static uint32_t _test_tick_counter = 0;

static void _test_explosion_tick(void) {
  _test_tick_counter++;

  // Explode ship every 3 seconds (360 ticks at 120Hz)
  if (_test_tick_counter >= 360) {
    _test_tick_counter = 0;
    // Ship is at object index 0
    explode_entity(0);
  }
}

int run(void) {
  bool running = true;

  debug_initialize();
  platform_initialize();
  messaging_initialize();
  entity_manager_initialize();
  graphics_initialize();
  collisions_engine_initialize();

  messaging_send(RECIPIENT_ID_BROADCAST, CREATE_MESSAGE(MESSAGE_BROADCAST_SYSTEM_INITIALIZED, 0, 0));

  while (running) {
    platform_frame_start();

    PROFILE_FRAME_START("Platform loop");
    running = platform_loop();
    PROFILE_FRAME_END("Platform loop");

    if (!running)
      break;

    PROFILE_FRAME_START("Physics");
    messaging_send(RECIPIENT_ID_BROADCAST, CREATE_MESSAGE(MESSAGE_BROADCAST_FRAME_TICK, 0, 0));

    while (platform_tick_pending()) {
      _test_explosion_tick();  // TEST
      physics_engine_tick();
      collisions_engine_tick();
      messaging_pump();
    }
    PROFILE_FRAME_END("Physics");

    PROFILE_FRAME_START("Render");
    graphics_engine_draw();
    debug_watch_draw();
    PROFILE_FRAME_END("Render");

    platform_renderer_report_stats();
    platform_frame_end();
    PROFILE_FRAME_MARK();
  }
  return 0;
}

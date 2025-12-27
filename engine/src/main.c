#include "platform/platform.h"
#include "entity/entity.h"
#include "physics/physics.h"
#include "graphics/graphics.h"

int run(void) {
  bool running = true;

  platform_initialize();
  _entity_manager_initialize();

  while (running) {
    platform_frame_start();
    running = platform_loop();

    if (!running) // fail fast
      break;

    while (platform_tick_pending()) {
      // update game
      physics_engine_tick();
    }

    graphics_engine_draw();

    platform_frame_end();
  }
  return 0;
}

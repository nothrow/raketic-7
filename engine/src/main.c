#include "platform/platform.h"
#include "entity/entity.h"
#include "physics/physics.h"

int __cdecl main(void) {
  bool running = true;

  memory_initialize();
  entity_manager_initialize();

  platform_create_window();

  while (running) {
    platform_frame_start();

    running = platform_loop();

    while (platform_tick_pending()) {
      // update game

      physics_engine_tick();

    }

    platform_frame_end();
  }
  return 0;
}

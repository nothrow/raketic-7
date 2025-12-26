#include "platform/platform.h"
#include "entity/entity.h"

int main(void) {
  bool running = true;

  memory_initialize();
  entity_manager_initialize();

  platform_create_window();

  while (running) {
    platform_frame_start();

    running = platform_loop();

    while (platform_is_dirty()) {
      // update game
    }

    platform_frame_end();
  }
  return 0;
}

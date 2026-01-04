#include <Windows.h>
#include "unity.h"

#pragma comment(lib, "opengl32.lib")

void _memory_initialize(void);
void _gl_initialize(void);
void _math_initialize(void);

#include "entity/types.h"
#include "messaging/messaging.h"

void platform_initialize(void) {
}

const struct input_state* platform_get_input_state(void) {
  _ASSERT(0 && "not implemented");
  return NULL;
}

void platform_frame_start(void) {
  _ASSERT(0 && "not implemented");
}

void platform_frame_end(void) {
  _ASSERT(0 && "not implemented");
}

bool platform_tick_pending(void) {
  _ASSERT(0 && "not implemented");
  return false;
}

bool platform_loop(void) {
  _ASSERT(0 && "not implemented");
  return false;
}

bool platform_input_is_button_down(void) {
  _ASSERT(0 && "not implemented");
  return false;
}

void* platform_retrieve_memory(size_t memory_size) {
  return _aligned_malloc(memory_size, 16);
}

void entity_manager_initialize(void);

void setUp(void) {
  entity_manager_initialize();
}

void tearDown(void) {}

void physics_test__parts_world_transform_rotations(void);

int __cdecl main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  UNITY_BEGIN();
  RUN_TEST(physics_test__parts_world_transform_rotations);
  return UNITY_END();
}

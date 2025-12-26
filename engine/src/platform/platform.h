#pragma once

#include <stdbool.h>

#include <assert.h>
#include <stdbool.h>

#define _ASSERT assert

#ifdef NDEBUG
#define _VERIFY(x, msg) (x)
#else
#define _VERIFY(x, msg) assert((x) && (msg))
#endif


#define TICK_MS (1000 / 120.0)
#define TICK_S (1000 / 120.0) / 1000.0

void clear_memory(void* ptr, size_t size);

struct input_state {
  int mx, my;
  int mdx, mdy;

  unsigned int buttons;
};

void platform_create_window(void);
int platform_loop(void);
bool platform_tick_pending(void);

void platform_frame_start(void);
void platform_frame_end(void);

const struct input_state* platform_get_input_state(void);

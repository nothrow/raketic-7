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


void clear_memory(void* ptr, size_t size);

struct input_state {
  int mx, my;
  int mdx, mdy;

  unsigned int buttons;
};

void platform_create_window(void);
int platform_loop(void);
bool platform_is_dirty(void);

void platform_frame_start(void);
void platform_frame_end(void);

const struct input_state* platform_get_input_state(void);

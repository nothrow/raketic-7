#pragma once

#include "core/core.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

struct input_state {
  int mx, my;
  int mdx, mdy;
  unsigned int buttons;
};

void platform_initialize(void);
bool platform_loop(void);
bool platform_tick_pending(void);

void platform_frame_start(void);
void platform_frame_end(void);

void platform_renderer_draw_models(
  size_t model_count,
  const color_t* colors,
  const position_orientation_t* position_orientation,
  const uint16_t* model_indices
);

const struct input_state* platform_get_input_state(void);

// Memory management (fixed heap, no dynamic allocation)
// the returned memory is aligned to 16 bytes
// no assumptions about the memory being cleared are done
void* platform_retrieve_memory(size_t memory_size);
void platform_clear_memory(void* ptr, size_t size);

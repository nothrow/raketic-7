#pragma once

#include "core/core.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

enum buttons {
  BUTTON_LEFT = 0x1,
  BUTTON_RIGHT = 0x2,
  BUTTON_MIDDLE = 0x4,
};

enum keys {
  KEY_SPACE = 0x20,
  KEY_TILDE = 0xC0, // VK_OEM_3 (~/` key)
  KEY_COUNT = 0xC1,
};


struct input_state {
  int mx, my;
  int mdx, mdy;
  unsigned int buttons;
  uint8_t keyPressed[KEY_COUNT];
};


void platform_initialize(void);
bool platform_loop(void);
bool platform_tick_pending(void);

void platform_frame_start(void);
void platform_frame_end(void);

void platform_renderer_draw_models(size_t model_count, const color_t* colors,
                                   const position_orientation_t* position_orientation, const uint16_t* model_indices);

const struct input_state* platform_get_input_state(void);
bool platform_input_is_button_down(enum buttons button);
bool platform_input_is_key_down(enum keys key);

// Memory management (fixed heap, no dynamic allocation)
// the returned memory is aligned to 16 bytes
// no assumptions about the memory being cleared are done
void* platform_retrieve_memory(size_t memory_size);
void platform_clear_memory(void* ptr, size_t size);

void platform_debug_draw_line(float x1, float y1, float x2, float y2, color_t color);

// Star field rendering (vertices = interleaved x,y pairs)
void platform_renderer_draw_stars(size_t count, const float* vertices, const color_t* colors);

// Beam rendering (laser beams with fade effect)
void platform_renderer_draw_beams(size_t count, const float* start_x, const float* start_y,
                                  const float* end_x, const float* end_y,
                                  const uint16_t* lifetime_ticks, const uint16_t* lifetime_max);

// Call at end of frame to report draw call stats to profiler
void platform_renderer_report_stats(void);

// HUD line rendering (non-debug, always available)
void platform_renderer_draw_line(float x1, float y1, float x2, float y2, color_t color);

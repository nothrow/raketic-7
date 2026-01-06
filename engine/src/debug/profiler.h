#pragma once

//
// Tracy Profiler integration
//
// Usage:
//   PROFILE_ZONE("name")        - measures current scope with given name
//   PROFILE_PLOT("name", value) - plots a value (e.g. object count)
//   PROFILE_FRAME_MARK()        - marks frame boundary (call once per frame)
//
// To enable: define TRACY_ENABLE before including this header (or in build config)
// When disabled: all macros expand to nothing (zero overhead, not in binary)
//

#ifdef TRACY_ENABLE

#include <tracy/TracyC.h>

// Zone macro using dynamic source location allocation
// This works with MSVC C mode where __func__ is not a constant expression
// Note: name must be a string literal for sizeof() to work correctly
#define PROFILE_ZONE(name) \
    TracyCZoneCtx __tracy_ctx = ___tracy_emit_zone_begin_alloc( \
        ___tracy_alloc_srcloc_name((uint32_t)__LINE__, __FILE__, sizeof(__FILE__) - 1, \
            name, sizeof(name) - 1, name, sizeof(name) - 1, 0), 1)

#define PROFILE_ZONE_END() ___tracy_emit_zone_end(__tracy_ctx)

// Add extra info to current zone
#define PROFILE_ZONE_VALUE(val)      ___tracy_emit_zone_value(__tracy_ctx, (uint64_t)(val))
#define PROFILE_ZONE_TEXT(txt, len)  ___tracy_emit_zone_text(__tracy_ctx, txt, len)
#define PROFILE_ZONE_COLOR(color)    ___tracy_emit_zone_color(__tracy_ctx, color)

// Predefined colors (0xRRGGBB)
#define PROFILE_COLOR_PHYSICS   0x3498db  // blue
#define PROFILE_COLOR_RENDER    0x2ecc71  // green
#define PROFILE_COLOR_ENTITY    0xe74c3c  // red
#define PROFILE_COLOR_MESSAGE   0x9b59b6  // purple
#define PROFILE_COLOR_AUDIO     0xf39c12  // orange

// Plot values over time (great for tracking object counts, etc.)
#define PROFILE_PLOT(name, val)   ___tracy_emit_plot(name, (double)(val))
#define PROFILE_PLOT_I(name, val) ___tracy_emit_plot_int(name, (int64_t)(val))

// Frame boundary markers
#define PROFILE_FRAME_MARK()           ___tracy_emit_frame_mark(0)
#define PROFILE_FRAME_MARK_NAMED(name) ___tracy_emit_frame_mark(name)

// Use for separate frame types (e.g. physics tick vs render frame)
#define PROFILE_FRAME_START(name) ___tracy_emit_frame_mark_start(name)
#define PROFILE_FRAME_END(name)   ___tracy_emit_frame_mark_end(name)

// Message logging - shows in timeline
#define PROFILE_MESSAGE(msg)              ___tracy_emit_messageL(msg, 0)
#define PROFILE_MESSAGE_COLOR(msg, color) ___tracy_emit_messageLC(msg, color, 0)

// Memory tracking - shows allocations in timeline
#define PROFILE_ALLOC(ptr, size)  ___tracy_emit_memory_alloc(ptr, size, 0)
#define PROFILE_FREE(ptr)         ___tracy_emit_memory_free(ptr, 0)

// App info - shows in Tracy header
#define PROFILE_APP_INFO(txt, len) ___tracy_emit_message_appinfo(txt, len)

// Draw call counting - use in renderer
static size_t __profile_draw_calls = 0;
#define PROFILE_DRAW_CALL()       __profile_draw_calls++
#define PROFILE_DRAW_CALLS(n)     __profile_draw_calls += (n)
#define PROFILE_DRAW_CALLS_RESET() do { \
    ___tracy_emit_plot_int("draw_calls", (int64_t)__profile_draw_calls); \
    __profile_draw_calls = 0; \
  } while(0)

#else

// When profiling is disabled, all macros expand to nothing
#define PROFILE_ZONE(name)
#define PROFILE_ZONE_END()
#define PROFILE_ZONE_VALUE(val)
#define PROFILE_ZONE_TEXT(txt, len)
#define PROFILE_ZONE_COLOR(color)
#define PROFILE_COLOR_PHYSICS   0
#define PROFILE_COLOR_RENDER    0
#define PROFILE_COLOR_ENTITY    0
#define PROFILE_COLOR_MESSAGE   0
#define PROFILE_COLOR_AUDIO     0
#define PROFILE_PLOT(name, val)
#define PROFILE_PLOT_I(name, val)
#define PROFILE_FRAME_MARK()
#define PROFILE_FRAME_MARK_NAMED(name)
#define PROFILE_FRAME_START(name)
#define PROFILE_FRAME_END(name)
#define PROFILE_MESSAGE(msg)
#define PROFILE_MESSAGE_COLOR(msg, color)
#define PROFILE_ALLOC(ptr, size)
#define PROFILE_FREE(ptr)
#define PROFILE_APP_INFO(txt, len)
#define PROFILE_DRAW_CALL()
#define PROFILE_DRAW_CALLS(n)
#define PROFILE_DRAW_CALLS_RESET()

#endif

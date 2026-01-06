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

// Plot values over time (great for tracking object counts, etc.)
#define PROFILE_PLOT(name, val)   ___tracy_emit_plot(name, (double)(val))
#define PROFILE_PLOT_I(name, val) ___tracy_emit_plot_int(name, (int64_t)(val))

// Frame boundary marker
#define PROFILE_FRAME_MARK() ___tracy_emit_frame_mark(0)

// Message logging
#define PROFILE_MESSAGE(msg) ___tracy_emit_messageL(msg, 0)

#else

// When profiling is disabled, all macros expand to nothing
#define PROFILE_ZONE(name)
#define PROFILE_ZONE_END()
#define PROFILE_PLOT(name, val)
#define PROFILE_PLOT_I(name, val)
#define PROFILE_FRAME_MARK()
#define PROFILE_MESSAGE(msg)

#endif

---
name: performance-profiling
description: Performance profiling with Tracy and optimization techniques for Raketic
---
# Performance Profiling in Raketic

## Overview

Raketic uses Tracy profiler for real-time performance analysis in debug builds, with ETW (Event Tracing for Windows) as a secondary option.

## Tracy Profiler

### Setup

Tracy is included via vcpkg. The profiler client (TracyClient.dll) is loaded in debug builds.

### Basic Instrumentation

```c
#include "debug/profiler.h"

void my_function(void) {
  ZoneScoped;  // Automatic zone for this function

  // ... code ...
}
```

### Named Zones

```c
void complex_function(void) {
  {
    ZoneScopedN("Physics Update");
    // physics code
  }

  {
    ZoneScopedN("Render Prep");
    // render prep code
  }
}
```

### Colored Zones

```c
ZoneScopedNC("Hot Path", 0xFF0000);  // Red
ZoneScopedNC("Cold Path", 0x0000FF); // Blue
```

### Frame Marks

```c
// Call once per frame in main loop
FrameMark;
```

### Memory Tracking

```c
TracyAlloc(ptr, size);
TracyFree(ptr);
```

### Plot Values

```c
TracyPlot("Particle Count", (float)particles->active);
TracyPlot("Frame Time", dt_ms);
```

## Running Tracy Profiler

1. Build in Debug or ReleaseSDL configuration
2. Run the game (`engine.exe`)
3. Launch Tracy profiler GUI
4. Connect to `localhost`

## Key Metrics to Watch

| Metric | Target | Issue if Exceeded |
|--------|--------|-------------------|
| Frame time | < 8.33ms (120 Hz) | Stuttering |
| Physics update | < 2ms | Simulation lag |
| Render time | < 4ms | Visual hitching |
| Memory | Static budget | Overflow/crash |

## Optimization Checklist

### CPU Hotspots

1. **Profile first** - don't guess
2. **Check loop counts** - O(n²) problems
3. **SIMD candidates** - batch float operations
4. **Cache misses** - data layout (SoA vs AoS)
5. **Branch mispredictions** - consider branchless

### Common Culprits in Game Engines

```c
// BAD: Division in hot loop
for (int i = 0; i < n; i++) {
  result[i] = value / divisor;  // Expensive!
}

// GOOD: Multiply by reciprocal
float inv_divisor = 1.0f / divisor;
for (int i = 0; i < n; i++) {
  result[i] = value * inv_divisor;
}
```

```c
// BAD: Function call per element
for (int i = 0; i < n; i++) {
  result[i] = sinf(angles[i]);
}

// GOOD: Use LUT (see platform/math.c)
for (int i = 0; i < n; i++) {
  result[i] = lut_sin(angles_deg[i]);
}
```

## Size Profiling (64kB constraint)

### Check Executable Size

```powershell
(Get-Item ReleaseMin\engine.exe).Length / 1KB
# Target: < 64 KB
```

### Size Contributors

1. **Static data** - LUTs, strings, model vertices
2. **Code** - functions, especially inlined
3. **Generated code** - watch model complexity

### Size Reduction Techniques

- Use lookup tables instead of math functions
- Share common code paths
- Minimize string literals
- Compress or simplify model data
- Link-time optimization (LTO)

## Debug vs Release Builds

| Config | Tracy | Optimizations | Use Case |
|--------|-------|---------------|----------|
| Debug | Yes | None | Development |
| Release | No | Full | Testing |
| ReleaseSDL | Yes | Full | Profiling optimized code |
| ReleaseMin | No | Size-optimized | Final 64kB build |

## ETW (Windows Event Tracing)

Alternative to Tracy, built into Windows:

```c
// In debug builds
#include <evntprov.h>

// Use Windows Performance Analyzer (WPA) to view traces
```

Useful for correlating with system-wide events (GPU, disk, etc.).

# Raketic-7 - 2D Space Game Engine

## Purpose
A 2D space game engine written in pure C with SIMD (AVX2) optimizations. Features ships, planets, weapons (lasers, rockets), particle effects, and Newtonian physics with gravity.

## Tech Stack
- **Language**: C (compiled as C with MSVC)
- **Build System**: Visual Studio 2022 solution (`.sln` + `.vcxproj`)
- **Dependencies**: SDL2, Tracy profiler (via vcpkg)
- **Platforms**: Multiple render backends - Win32/GDI (`platform.win.c`), SDL2/OpenGL (`platform.sdl.c`), minimalist (`platform.min.c`)
- **Architecture**: x86 (32-bit) with AVX2 SIMD, also x64 configs available

## Key Architecture
- **Entity System**: SoA (Struct of Arrays) layout for cache-friendly SIMD processing
  - `objects_data`: Main entities (ships, rockets, planets) with physics
  - `parts_data`: Sub-entities attached to objects (engines, weapons)
  - `particles_data`: Lightweight visual-only particles
- **ID System**: entity_id_t packs type (8 bits) + ordinal (23 bits), with part flag bit 23
- **Messaging**: Queue-based message dispatch with broadcast support
- **Physics**: Yoshida 4th-order symplectic integrator for objects, Euler for particles, N-body gravity
- **Collisions**: SIMD broad-phase with frustum culling, circle-circle tests
- **Data Pipeline**: Lua data files -> C# modelgen tool -> generated C code (models, maps)
- **Models**: SVG files parsed by modelgen, with slot system for attaching parts

## Code Structure
```
engine/src/
  core/         - core.h (types, timing), vector.c
  entity/       - entity system, all entity types
  collisions/   - SIMD collision detection
  physics/      - Yoshida integrator, gravity, particle physics
  graphics/     - rendering orchestration, star field
  messaging/    - message queue
  platform/     - platform abstraction (win32, SDL, minimalist)
  debug/        - debug overlay, profiler integration
engine/generated/ - auto-generated from data/ by modelgen
data/
  models/       - SVG model files
  entities/     - Lua entity definitions
  parts/        - Lua part definitions
  worlds/       - Lua world/map definitions
```

## Entity Types (enum entity_type in types.h)
- ENTITY_TYPE_CONTROLLER (1), CAMERA (2), PARTICLES (3), SHIP (4)
- ENTITY_TYPE_PART_ENGINE (5), PART_WEAPON (6), PLANET (7), ROCKET (8), BEAMS (9)
- ENTITY_TYPE_COUNT = 10

## Health System
- objects_data has `int16_t* health` - <=0 means dead
- `entity_manager_pack_objects()` sweeps dead objects, remaps references
- Rockets auto-destruct by setting health=0 when lifetime expires

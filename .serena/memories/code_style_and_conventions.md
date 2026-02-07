# Code Style and Conventions

## Formatting
- .clang-format based on LLVM style
- 2-space indentation, no tabs
- K&R braces (opening on same line)
- Pointer attached to type: `float* ptr`
- Column limit: 120 characters
- No sorted includes

## Naming Conventions
- Snake_case for functions and variables
- UPPER_CASE for macros/constants
- Prefix with underscore for static/internal functions: `_rocket_spawn_smoke`
- Module prefix for public functions: `entity_manager_`, `rocket_`, `beams_`, `particles_`
- Type suffix `_t` for typedefs: `entity_id_t`, `message_t`
- Static module-level data with trailing underscore: `manager_`, `rocket_aux_`, `beams_`

## Patterns
- SoA (Struct of Arrays) for all bulk data
- Each entity type has: `xxx_entity_initialize()` that registers vtable
- Vtable has single `dispatch_message` function pointer
- Memory: `platform_retrieve_memory()` for arena-style allocation (no free)
- Data in `_128bytes` union for parts (type-punned via cast)
- SIMD: Process 8 floats at a time (AVX2), pad arrays to multiples of 8
- Compact/pack dead entries by swapping with alive from end

## File Organization
- Each entity type: `.h` (public API + data structs) and `.c` (implementation)
- `entity_internal.h` for shared vtable declaration
- Generated code in `engine/generated/`

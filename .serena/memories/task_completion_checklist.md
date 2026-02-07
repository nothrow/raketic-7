# Task Completion Checklist

1. Follow existing code style (snake_case, 2-space indent, K&R braces)
2. Register new entity types in `types.h` enum and call initialize in `entity.c`
3. For new models: create SVG in `data/models/`, update modelgen if needed
4. For new entities: create Lua data file in `data/entities/`
5. Ensure SoA arrays are padded to multiples of 8 for SIMD
6. Add remap handling if new entity holds object indices
7. New data files require modelgen re-run to update generated code

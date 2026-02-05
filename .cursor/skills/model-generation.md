# Model Generation Pipeline

## Overview

Raketic uses a C# tool (`raketic.modelgen`) to convert SVG models and Lua data into C code compiled into the executable.

## Pipeline Flow

```
SVG Models (data/models/*.svg)
         ↓
Lua Entity Defs (data/entities/*.lua)
         ↓
Lua World Defs (data/worlds/*.lua)
         ↓
    raketic.modelgen (C#)
         ↓
Generated C Code (engine/generated/*.gen.c/h)
```

## Running the Generator

```powershell
cd raketic.modelgen
dotnet run
```

## Generated Files

| File | Contents |
|------|----------|
| `renderer.gen.h` | `MODEL_*_IDX` constants, function declarations |
| `renderer.gen.c` | `_generated_draw_model()` implementation |
| `models.gen.c` | Model vertex data, drawing commands |
| `models_meta.gen.c` | Model metadata (radius, slots) |
| `slots.gen.h` | Slot definitions from SVG |

## Creating SVG Models

### Requirements

- Wireframe only (no fills)
- White strokes: `stroke="white"`
- No fill: `fill="none"`
- Reasonable size (will be normalized)

### Basic Template

```xml
<?xml version="1.0" encoding="UTF-8"?>
<svg width="100" height="100" viewBox="0 0 100 100"
     xmlns="http://www.w3.org/2000/svg">

  <!-- Optional: explicit center point -->
  <circle id="center" cx="50" cy="50" r="1" />

  <!-- Model geometry -->
  <path d="M 50,10 L 90,90 L 10,90 Z"
        fill="none" stroke="white" stroke-width="2"/>
</svg>
```

### Slot System

Define attachment points for parts using `slot` attribute:

```xml
<g slot="engine1" transform="translate(30, 80)">
  <!-- Slot position marker -->
</g>
<g slot="engine2" transform="translate(70, 80)">
  <!-- Another slot -->
</g>
```

Slots are referenced in Lua entity definitions.

## Lua Entity Definitions

### Entity File (`data/entities/myEntity.lua`)

```lua
return Ship {
  model  = models.myModel,
  mass   = 1,
  radius = models.myModel.radius,
  slots = {
    engine1 = parts.basicEngine,
    engine2 = parts.basicEngine
  }
}
```

### Part File (`data/parts/myPart.lua`)

```lua
return Part {
  model = models.engine,
  type  = "engine",
  thrust = 100
}
```

## Lua World Definitions

### World File (`data/worlds/myWorld.lua`)

```lua
return World {
  entities = {
    { entity = entities.basicShip, x = 0, y = 0 },
    { entity = entities.planet, x = 1000, y = 0 }
  }
}
```

## Using Generated Code

### Drawing Models

```c
#include "generated/renderer.gen.h"

// Draw at current transform
_generated_draw_model(color, MODEL_SHIP_IDX);
```

### Model Constants

```c
MODEL_SHIP_IDX      // uint16_t index
MODEL_ENGINE_IDX
MODEL_PLANET_IDX
// etc.
```

### Getting Model Radius

```c
uint16_t radius = _generated_get_model_radius(MODEL_SHIP_IDX);
```

## Troubleshooting

### Model Not Appearing

1. Check SVG is in `data/models/`
2. Verify SVG has `stroke="white"` paths
3. Re-run `dotnet run` in modelgen folder
4. Check for parse errors in console output

### Slot Not Working

1. Verify `slot="name"` attribute on SVG element
2. Match slot name exactly in Lua entity definition
3. Regenerate code after SVG changes

# Lua Data Authoring for Raketic

## Overview

Raketic uses Lua as a DSL (Domain-Specific Language) for defining game content. The Lua files are processed by `raketic.modelgen` and compiled into C code.

## File Locations

```
data/
├── entities/     # Entity definitions (ships, planets)
├── models/       # SVG model files
├── parts/        # Component definitions (engines, weapons)
└── worlds/       # World/level definitions
```

## Available Globals

The Lua environment provides these global tables:

| Global | Contents |
|--------|----------|
| `models` | All loaded SVG models by name |
| `entities` | All entity definitions |
| `parts` | All part definitions |

## Entity Definitions

### Ship Entity

```lua
-- data/entities/playerShip.lua
return Ship {
  model  = models.ship,
  mass   = 1.0,
  radius = models.ship.radius,
  slots = {
    engine1 = parts.basicEngine,
    engine2 = parts.basicEngine,
    weapon1 = parts.laser
  }
}
```

### Planet Entity

```lua
-- data/entities/earthLike.lua
return Planet {
  model  = models.planet,
  mass   = 1000000,
  radius = models.planet.radius,
  -- Planets don't have slots
}
```

### Available Entity Types

| Type | Description |
|------|-------------|
| `Ship` | Controllable/AI vessel with slots |
| `Planet` | Massive body with gravity |
| `Station` | Static structure with slots |
| `Asteroid` | Small body, can be destroyed |

## Part Definitions

### Engine Part

```lua
-- data/parts/basicEngine.lua
return Part {
  model  = models.engine,
  type   = "engine",
  thrust = 100,
  fuel_consumption = 0.1
}
```

### Weapon Part

```lua
-- data/parts/laser.lua
return Part {
  model  = models.laser,
  type   = "weapon",
  damage = 10,
  heat_generation = 5,
  cooldown_ticks = 60  -- 0.5 seconds at 120Hz
}
```

## World Definitions

### Basic World

```lua
-- data/worlds/tutorial.lua
return World {
  name = "Tutorial System",
  entities = {
    -- Player ship at origin
    {
      entity = entities.playerShip,
      x = 0,
      y = 0,
      vx = 0,
      vy = 0
    },
    -- Planet
    {
      entity = entities.earthLike,
      x = 5000,
      y = 0,
      vx = 0,
      vy = 0
    },
    -- Orbiting station
    {
      entity = entities.station,
      x = 5500,
      y = 0,
      vx = 0,
      vy = 31.6  -- Orbital velocity for circular orbit
    }
  }
}
```

### Orbital Placement Helper

For circular orbits around a body:

```lua
-- Orbital velocity: v = sqrt(G * M / r)
-- G ≈ 1 in game units, so v = sqrt(M / r)

local function orbital_velocity(parent_mass, distance)
  return math.sqrt(parent_mass / distance)
end

-- Example: moon orbiting planet at distance 1000
local planet_mass = 1000000
local moon_distance = 1000
local moon_v = orbital_velocity(planet_mass, moon_distance)

return World {
  entities = {
    { entity = entities.planet, x = 0, y = 0 },
    {
      entity = entities.moon,
      x = moon_distance,
      y = 0,
      vx = 0,
      vy = moon_v
    }
  }
}
```

## Model Properties

Models loaded from SVG expose:

```lua
models.ship.radius    -- Bounding radius (auto-calculated)
models.ship.slots     -- Table of slot names and positions
```

## Common Patterns

### Reusable Templates

```lua
-- data/entities/common.lua (if needed)
local BaseShip = {
  mass = 1.0
}

-- Use in other files by requiring
```

### Computed Values

```lua
return Ship {
  model = models.heavyShip,
  mass = 5.0,
  radius = models.heavyShip.radius * 1.2,  -- 20% larger hitbox
  slots = {
    -- Fill all slots from model
    engine1 = parts.heavyEngine,
    engine2 = parts.heavyEngine,
    engine3 = parts.heavyEngine,
    engine4 = parts.heavyEngine
  }
}
```

## Validation

The modelgen tool validates:

- ✓ Referenced models exist
- ✓ Referenced parts exist
- ✓ Slot names match SVG definitions
- ✓ Required properties are present

Errors are reported during `dotnet run`.

## Tips

1. **Keep masses relative** - Use 1.0 for standard ship, scale from there
2. **Match slot names** - Must exactly match SVG `slot="name"` attributes
3. **Test orbits** - Use Hill sphere calculations for stable orbits
4. **Iterate quickly** - Run modelgen after each change to catch errors

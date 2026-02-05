# Entity Creation in Raketic Engine

## Overview

Guide for adding new entity types to the Raketic engine's ECS-like architecture.

## Entity Categories

| Category | Storage | Lifetime | Examples |
|----------|---------|----------|----------|
| **Objects** | `objects_data` | Persistent | ship, planet |
| **Parts** | `parts_data` | Attached to objects | engine, weapon |
| **Particles** | `particles_data` | Short-lived, tick-based | exhaust, spark |

## Step-by-Step: New Entity Type

### 1. Register Type in `types.h`

```c
// engine/src/entity/types.h
enum entity_type {
  ENTITY_TYPE_ANY = 0,
  // ... existing ...
  ENTITY_TYPE_MY_ENTITY,    // <-- add before COUNT
  ENTITY_TYPE_COUNT,
};

static entity_type_t ENTITY_TYPEREF_MY_ENTITY = { ENTITY_TYPE_MY_ENTITY };
```

### 2. Create Header File

```c
// engine/src/entity/my_entity.h
#pragma once
#include "entity_internal.h"

void my_entity_entity_initialize(void);
```

### 3. Create Implementation

```c
// engine/src/entity/my_entity.c
#include "platform/platform.h"
#include "platform/math.h"
#include "entity_internal.h"
#include "entity.h"
#include "my_entity.h"

static void _my_entity_dispatch(entity_id_t id, message_t msg) {
  switch (msg.message) {
    case MESSAGE_BROADCAST_120HZ_BEFORE_PHYSICS:
      // Per-tick logic here
      break;
  }
}

void my_entity_entity_initialize(void) {
  entity_manager_vtables[ENTITY_TYPE_MY_ENTITY].dispatch_message = _my_entity_dispatch;
}
```

### 4. Register in Entity Manager

```c
// engine/src/entity/entity.c

#include "my_entity.h"  // Add include

static void _entity_manager_types_initialize(void) {
  // ... existing ...
  my_entity_entity_initialize();  // Add call
}
```

### 5. Add to Visual Studio Project

Edit `engine/engine.vcxproj`:

- Add `.c` file to `<ClCompile>` ItemGroup
- Add `.h` file to `<ClInclude>` ItemGroup

## Spawning Entities

```c
// For entities with models
entity_id_t id = _generate_entity_by_model(ENTITY_TYPEREF_MY_ENTITY, MODEL_MY_ENTITY_IDX);

// Access entity data
struct objects_data* objects = entity_manager_get_objects();
uint32_t idx = id.index;
objects->velocity_x[idx] = 10.0f;
```

## Message System

Entities communicate via async message bus:

### Broadcast Messages (received by all)

- `MESSAGE_BROADCAST_120HZ_BEFORE_PHYSICS` - update logic
- `MESSAGE_BROADCAST_120HZ_AFTER_PHYSICS` - post-physics reactions

### Custom Messages

Define in `messaging/messaging.h`:

```c
enum message_type {
  // ... existing ...
  MESSAGE_MY_CUSTOM_EVENT,
};
```

Send targeted message:

```c
message_t msg = { .message = MESSAGE_MY_CUSTOM_EVENT, .data = { ... } };
entity_manager_dispatch_message(target_id, msg);
```

## Parts (Attached Components)

Parts are stored separately but linked to parent objects:

```c
struct parts_data {
  entity_id_t* parent_id;     // Link to owner
  float* local_offset_x;      // Offset from parent center
  float* local_offset_y;
  struct _128bytes* data;     // Part-specific data (128 bytes max)
};
```

## Particles

Particles use tick-based lifetime:

```c
struct particles_data {
  uint16_t* lifetime_ticks;   // Current age
  uint16_t* lifetime_max;     // Max lifetime
  // Auto-removed when lifetime_ticks >= lifetime_max
};
```

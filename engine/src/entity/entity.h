#pragma once

#include <stdint.h>
#include "core/core.h"
#include "messaging/messaging.h"

typedef uint32_t entity_id_t;

#include "types.h"

// either id (if raw), or lookup
entity_id_t entity_manager_lookup_raw(entity_id_t id);
// either id, or lookup, so it has type
entity_id_t entity_manager_lookup_typed(entity_id_t id);

struct objects_data {
  uint32_t active;
  uint32_t capacity;

  float* velocity_x;
  float* velocity_y;
  float* thrust;
  entity_id_t* identifiers;

  position_orientation_t position_orientation;

  uint16_t* model_idx;

  float* mass;
  float* radius;
};

struct _128bytes {
  uint8_t data[128];
};

struct particles_data {
  uint32_t active;
  uint32_t capacity;

  float* velocity_x;
  float* velocity_y;

  uint16_t* lifetime_ticks;
  uint16_t* lifetime_max;
  uint16_t* model_idx;

  position_orientation_t position_orientation;

  struct _128bytes* temporary; // reserved memory for intermediate computations
};

void entity_manager_initialize(void);
struct particles_data* entity_manager_get_particles(void);
struct objects_data* entity_manager_get_objects(void);
void entity_manager_pack_particles(void);

void entity_manager_dispatch_message(messaging_recipient_type_t recipient_type, messaging_recipient_id_t recipient_id, message_t msg);




#pragma once

#include <stdint.h>

#include "types.h"
#include "core/core.h"
#include "messaging/messaging.h"

struct objects_data {
  uint32_t active;
  uint32_t capacity;

  float* velocity_x;
  float* velocity_y;

  float* acceleration_x;
  float* acceleration_y;

  float* thrust;
  entity_type_t* type;

  position_orientation_t position_orientation;

  uint32_t* parts_start_idx;
  uint32_t* parts_count;

  uint16_t* model_idx;

  float* mass;
};

struct parts_data {
  uint32_t active;
  uint32_t capacity;

  entity_id_t* parent_id;
  entity_type_t* type;

  float* local_offset_x;
  float* local_offset_y;
  float* local_orientation_x;
  float* local_orientation_y;

  struct _128bytes* data;

  position_orientation_t world_position_orientation;

  uint16_t* model_idx;
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
struct parts_data* entity_manager_get_parts(void);

void entity_manager_get_vectors(entity_id_t entity_id, float* pos, float* vel);

void entity_manager_pack_particles(void);

void entity_manager_dispatch_message(entity_id_t recipient_id, message_t msg);
entity_id_t entity_manager_resolve_object(uint32_t ordinal);

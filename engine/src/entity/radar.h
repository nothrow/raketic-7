#pragma once

#include "entity_internal.h"

#define MAX_RADAR_CONTACTS 8
#define MAX_NAV_CONTACTS   2

struct radar_contact {
  float x;
  float y;
  uint16_t entity_ordinal;    // index into objects_data
  uint8_t entity_type;        // FoE identification: what kind of entity is this
  uint8_t _pad;
};

// sizeof(radar_data) = 4 + 4 + 8*12 + 2*12 = 128 bytes  (fills 128-byte part data)
struct radar_data {
  float range;                                    // detection range
  uint8_t contact_count;                          // how many tactical contacts this tick
  uint8_t nav_count;                              // how many celestial body contacts
  uint8_t _pad[2];
  struct radar_contact contacts[MAX_RADAR_CONTACTS]; // tactical (sorted by distance, closest first)
  struct radar_contact nav[MAX_NAV_CONTACTS];        // celestial bodies (sorted by distance)
};

void radar_part_entity_initialize(void);

// Query the radar data for an entity (finds its radar part).
// Returns NULL if the entity has no radar part.
const struct radar_data* radar_get_data(entity_id_t owner);

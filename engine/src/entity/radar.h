#pragma once

#include "entity_internal.h"

#define MAX_RADAR_CONTACTS 8

struct radar_contact {
  float x;
  float y;
  uint8_t entity_type;  // FoE identification: what kind of entity is this
  uint8_t _pad[3];
};

// sizeof(radar_data) = 4 + 4 + 8*12 = 104 bytes  (fits in 128-byte part data)
struct radar_data {
  float range;                                    // detection range
  uint8_t contact_count;                          // how many contacts this tick
  uint8_t _pad[3];
  struct radar_contact contacts[MAX_RADAR_CONTACTS]; // sorted by distance (closest first)
};

void radar_part_entity_initialize(void);

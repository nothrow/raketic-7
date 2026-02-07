#pragma once

#include "entity_internal.h"

struct weapon_data {
  uint8_t weapon_type;      // WEAPON_TYPE_LASER or WEAPON_TYPE_ROCKET
  uint8_t firing;           // is currently firing
  uint16_t cooldown_ticks;  // time between shots
  uint16_t cooldown_remaining;
  uint16_t projectile_model;
  uint16_t smoke_model;
  float projectile_speed;
};

void weapon_part_entity_initialize(void);

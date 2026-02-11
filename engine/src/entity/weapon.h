#pragma once

#include "entity_internal.h"

struct weapon_data {
  uint8_t weapon_type;      // WEAPON_TYPE_LASER, WEAPON_TYPE_ROCKET, or WEAPON_TYPE_TURRET_LASER
  uint8_t firing;           // is currently firing
  uint16_t cooldown_ticks;  // time between shots
  uint16_t cooldown_remaining;
  uint16_t projectile_model;
  uint16_t smoke_model;
  float projectile_speed;
  float rocket_thrust;          // thrust power while fuel burns
  uint16_t rocket_fuel_ticks;   // how long the motor burns
  uint16_t rocket_lifetime_ticks; // total lifetime before auto-destruct
  // Turret laser fields
  float target_x;               // world-space target position
  float target_y;
  float max_range;              // maximum engagement range
};

void weapon_part_entity_initialize(void);

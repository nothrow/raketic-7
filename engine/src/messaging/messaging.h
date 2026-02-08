#pragma once

#include <stdint.h>

#include "entity/types.h"

static const entity_id_t RECIPIENT_ID_BROADCAST = { 0xFFFFFFFF };
static const entity_type_t RECIPIENT_TYPE_ANY = { 0xFF };


typedef struct {
  uint16_t message;

  int32_t data_a;
  int32_t data_b;
  int64_t data_cd;
} message_t;


enum message_codes_system {
  MESSAGE_BROADCAST_SYSTEM_INITIALIZED = 0,
  MESSAGE_BROADCAST_120HZ_AFTER_PHYSICS = 1,
  MESSAGE_BROADCAST_FRAME_TICK = 2,
  MESSAGE_BROADCAST_120HZ_BEFORE_PHYSICS = 3,
  MESSAGE_COLLIDE_OBJECT_OBJECT = 4, // data_a = this entity id, data_b = other entity id
  MESSAGE_COLLIDE_OBJECT_PARTICLE = 5, // data_a = this entity id, data_b = particle id
  MESSAGE_BEAM_HIT = 6, // data_a = target entity_id, data_b = damage
  MESSAGE_COLLIDE_PART = 7 // data_a = hitting object entity_id, data_b = part entity_id
};

enum message_codes_ship {
  MESSAGE_SHIP_ROTATE_BY = 0x10, // data_a = delta angle in degrees
  MESSAGE_SHIP_ROTATE_TO =
      0x11, // data_a = target vector x * 65535 (fixed int), data_b = target vector y * 65535 (fixed int)
  MESSAGE_SHIP_ENGINES_THRUST = 0x12, // data_a = thrust percentage 0-100
  MESSAGE_WEAPON_FIRE = 0x13, // data_a = 1 (start firing) or 0 (stop firing)
};

static inline message_t CREATE_MESSAGE(uint16_t msg, int32_t data_a, int32_t data_b) {
  message_t ret = { .message = (msg), .data_a = (data_a), .data_b = (data_b) };
  return ret;
}

static inline uint32_t _f2i(float f) {
  uint32_t ret;
  float* fp = (float*)&ret;
  *fp = f;
  return ret;
}

static inline float _i2f(uint32_t i) {
  float ret;
  uint32_t* ip = (uint32_t*)&ret;
  *ip = i;
  return ret;
}

static inline message_t CREATE_MESSAGE_F(uint16_t msg, float x, float y) {
  message_t ret = { .message = (msg), .data_a = _f2i(x), .data_b = _f2i(y) };
  return ret;
}

void messaging_initialize(void);

void messaging_send(entity_id_t recipient_id, message_t msg);
void messaging_pump(void);

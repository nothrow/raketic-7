#pragma once

#include <stdint.h>

#include "entity/types.h"

static const entity_id_t RECIPIENT_ID_BROADCAST = { 0xFFFFFFFF };
static const entity_type_t RECIPIENT_TYPE_ANY = { 0xFF };


typedef struct {
  uint16_t message;

  int32_t data_a;
  int32_t data_b;
} message_t;


enum message_codes_system {
  MESSAGE_BROADCAST_SYSTEM_INITIALIZED = 0,
  MESSAGE_BROADCAST_120HZ_TICK = 1,
  MESSAGE_BROADCAST_FRAME_TICK = 2,
};

enum message_codes_ship {
  MESSAGE_SHIP_ROTATE_BY = 0x10,
  MESSAGE_SHIP_ENGINES_THRUST = 0x11
};

static inline message_t CREATE_MESSAGE(uint16_t msg, int32_t data_a, int32_t data_b) {
  message_t ret = { .message = (msg), .data_a = (data_a), .data_b = (data_b) };
  return ret;
}

void messaging_initialize(void);

void messaging_send(entity_id_t recipient_id, message_t msg);
void messaging_pump(void);

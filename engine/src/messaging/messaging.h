#pragma once

#include <stdint.h>


typedef uint32_t messaging_recipient_type_t;
typedef uint32_t messaging_recipient_id_t;

static const messaging_recipient_id_t RECIPIENT_ID_BROADCAST = 0xFFFFFFFF;
static const messaging_recipient_type_t RECIPIENT_TYPE_ANY = 0xFFFFFFFF;


typedef struct {
  uint16_t message;

  int32_t data_a;
  int32_t data_b;
} message_t;


enum message_codes_system {
  MESSAGE_BROADCAST_SYSTEM_INITIALIZED = 0,
  MESSAGE_BROADCAST_TICK = 1
};

enum message_codes_controller {
  MESSAGE_CONTROLLER_MOUSEMOVE = 0x10,
};

enum message_codes_ship {
  MESSAGE_SHIP_ROTATE_BY = 0x10,
};

static inline message_t CREATE_MESSAGE(uint16_t msg, int32_t data_a, int32_t data_b) {
  message_t ret = { .message = (msg), .data_a = (data_a), .data_b = (data_b) };
  return ret;
}

void messaging_initialize(void);

void messaging_send(messaging_recipient_type_t recipient_type, messaging_recipient_id_t recipient_id, message_t msg);
void messaging_pump(void);

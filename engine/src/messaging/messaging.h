#pragma once

#include <stdint.h>


typedef uint32_t messaging_recipient_type_t;
typedef uint32_t messaging_recipient_id_t;

static const messaging_recipient_id_t RECIPIENT_ID_BROADCAST = 0xFFFFFFFF;
static const messaging_recipient_type_t RECIPIENT_TYPE_BROADCAST = 0xFFFFFFFF;
static const messaging_recipient_type_t RECIPIENT_TYPE_UNKNOWN = 0xFFFFFFFF;


typedef struct {
  uint16_t message;

  uint32_t data;
} message_t;


enum message_codes {
  MESSAGE_BROADCAST_SYSTEM_INITIALIZED = 1,
};

static inline message_t CREATE_MESSAGE(uint16_t msg, uint32_t data) {
  message_t ret = { .message = (msg), .data = (data) };
  return ret;
}

void messaging_initialize(void);

void messaging_send(messaging_recipient_type_t recipient_type, messaging_recipient_id_t recipient_id, message_t msg);
void messaging_pump(void);

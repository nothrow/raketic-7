#include "messaging.h"
#include "entity/entity.h"
#include "platform/platform.h"
#include "debug/profiler.h"

#define MAX_MESSAGES 1024

struct message_with_recipient {
  message_t message;
  entity_id_t recipient_id;
};

struct messaging_system {
  size_t head;
  size_t tail;

  struct message_with_recipient* messages;
};

static struct messaging_system messaging_ = { 0 };

void messaging_initialize(void) {
  messaging_.messages = platform_retrieve_memory(sizeof(struct message_with_recipient) * MAX_MESSAGES);
}

void messaging_send(entity_id_t recipient_id, message_t msg) {
  size_t next_tail = (messaging_.tail + 1) % MAX_MESSAGES;
  _ASSERT(next_tail != messaging_.head); // overflow

  messaging_.messages[messaging_.tail].recipient_id = recipient_id;
  messaging_.messages[messaging_.tail].message = msg;
  messaging_.tail = next_tail;
}

void messaging_pump(void) {
  PROFILE_ZONE("messaging_pump");
  size_t message_count = 0;
  while (messaging_.head != messaging_.tail) {
    struct message_with_recipient* mwr = &messaging_.messages[messaging_.head];
    // process message here
    // for now, just discard
    messaging_.head = (messaging_.head + 1) % MAX_MESSAGES;

    entity_manager_dispatch_message(mwr->recipient_id, mwr->message);
    message_count++;
  }
  PROFILE_PLOT("message_count", message_count);
  PROFILE_ZONE_END();
}

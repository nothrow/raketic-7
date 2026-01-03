#pragma once

#include "entity.h"

typedef struct {
  // dispatch requires type already known
  void (*dispatch_message)(entity_id_t id, message_t msg);
} object_vtable_t;

extern object_vtable_t entity_manager_vtables[];

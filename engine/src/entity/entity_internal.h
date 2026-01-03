#pragma once

#include "entity.h"


#define IS_ANY_TYPE(id) (((id) & 0xFF000000) == 0)
#define IS_TYPE(id, type) (((id) & 0xFF000000) == ((type) << 24))
#define CREATE_ID_WITH_TYPE(rawid, type) ((((type) & 0xFF) << 24) | ((rawid) & 0x00FFFFFF))
#define GET_TYPE(id) (((id) >> 24) & 0xFF)
#define GET_REFID(id) ((id) & 0x00FFFFFF)

typedef struct {
  entity_id_t (*lookup_raw)(entity_id_t id);
  // dispatch requires type already known
  void (*dispatch_message)(entity_id_t id, message_t msg);
} object_vtable_t;

extern object_vtable_t entity_manager_vtables[];

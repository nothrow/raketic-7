
#include "platform/platform.h"
#include "entity_internal.h"
#include "entity.h"
#include "ship.h"

#define MAXSIZE 8192

typedef struct {
  uint32_t active;
  uint32_t capacity;

  entity_id_t* rawid; // index in entity manager
} ship_manager_t;


static ship_manager_t ship_manager_ = { 0 };

static entity_id_t _ship_lookup_raw(entity_id_t id) {
  _ASSERT(GET_TYPE(id) == ENTITY_TYPE_SHIP);

  uint32_t refid = GET_REFID(id);

  _ASSERT(refid < ship_manager_.active);
  return ship_manager_.rawid[refid];
}

void ship_entity_initialize(void)
{
  ship_manager_.capacity = MAXSIZE;
  ship_manager_.rawid = platform_retrieve_memory(sizeof(entity_id_t) * MAXSIZE);


  entity_manager_vtables[ENTITY_TYPE_SHIP].lookup_raw = _ship_lookup_raw;

  // will be deleted - just for testing
  ship_manager_.active = 1;
  ship_manager_.rawid[0] = 0;
}

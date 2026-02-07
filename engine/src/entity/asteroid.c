#include "asteroid.h"
#include "explosion.h"
#include "platform/platform.h"

#define ROCKET_DAMAGE 25


static void _asteroid_handle_collision(entity_id_t id, const message_t* msg) {
  entity_id_t id_a = { (uint32_t)msg->data_a };
  entity_id_t id_b = { (uint32_t)msg->data_b };

  // determine which is "other"
  entity_id_t other;
  if (GET_TYPE(id_a) == ENTITY_TYPEREF_ASTEROID._) {
    other = id_b;
  } else {
    other = id_a;
  }

  // only take damage from rockets
  if (GET_TYPE(other) != ENTITY_TYPEREF_ROCKET._) {
    return;
  }

  uint32_t self_idx = GET_ORDINAL(id);
  struct objects_data* od = entity_manager_get_objects();

  // already dead?
  if (od->health[self_idx] <= 0) return;

  od->health[self_idx] -= ROCKET_DAMAGE;

  if (od->health[self_idx] <= 0) {
    // death explosion proportional to mass
    float px = od->position_orientation.position_x[self_idx];
    float py = od->position_orientation.position_y[self_idx];
    float vx = od->velocity_x[self_idx];
    float vy = od->velocity_y[self_idx];
    float mass = od->mass[self_idx];

    explosion_spawn(px, py, vx, vy, mass);

    od->health[self_idx] = 0; // ensure dead for sweep
  }
}

static void _asteroid_dispatch(entity_id_t id, message_t msg) {
  switch (msg.message) {
  case MESSAGE_COLLIDE_OBJECT_OBJECT:
    _asteroid_handle_collision(id, &msg);
    break;
  }
}

static void _chunk_part_dispatch(entity_id_t id, message_t msg) {
  (void)id;
  (void)msg;
  // no-op: chunks are purely visual
}

void asteroid_entity_initialize(void) {
  entity_manager_vtables[ENTITY_TYPE_ASTEROID].dispatch_message = _asteroid_dispatch;
}

void chunk_part_entity_initialize(void) {
  entity_manager_vtables[ENTITY_TYPE_PART_CHUNK].dispatch_message = _chunk_part_dispatch;
}

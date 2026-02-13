#include "platform/platform.h"
#include "platform/math.h"
#include "radar.h"
#include "entity.h"

#include <math.h>

// Radar scans for all non-celestial objects within range.
// Contacts are stored sorted by distance (closest first) in radar_data,
// then MESSAGE_RADAR_SCAN_COMPLETE is sent to the parent entity.
// Each contact carries entity_type for Friend-or-Foe identification.

static bool _is_scannable_type(uint8_t type) {
  // Detect anything that isn't a planet, moon, beam, particle, camera, or controller
  switch (type) {
  case ENTITY_TYPE_ASTEROID:
  case ENTITY_TYPE_SHIP:
  case ENTITY_TYPE_ROCKET:
  case ENTITY_TYPE_SATELLITE:
    return true;
  default:
    return false;
  }
}

static void _radar_tick(void) {
  struct parts_data* pd = entity_manager_get_parts();
  struct objects_data* od = entity_manager_get_objects();

  for (uint32_t i = 0; i < pd->active; i++) {
    if (pd->type[i]._ != ENTITY_TYPE_PART_RADAR) continue;

    struct radar_data* rd = (struct radar_data*)(pd->data[i].data);
    entity_id_t parent_id = pd->parent_id[i];
    uint32_t parent_idx = GET_ORDINAL(parent_id);

    if (parent_idx >= od->active) continue;

    float sx = od->position_orientation.position_x[parent_idx];
    float sy = od->position_orientation.position_y[parent_idx];
    float range_sq = rd->range * rd->range;

    // Collect contacts: insertion-sort by distance into the contacts array.
    // We maintain up to MAX_RADAR_CONTACTS, sorted closest-first.
    // distances[] tracks the squared distance for each stored contact.
    uint8_t count = 0;
    float distances[MAX_RADAR_CONTACTS];

    for (uint32_t j = 0; j < od->active; j++) {
      if (j == parent_idx) continue; // skip self
      if (!_is_scannable_type(od->type[j]._)) continue;
      if (od->health[j] <= 0) continue;

      float dx = od->position_orientation.position_x[j] - sx;
      float dy = od->position_orientation.position_y[j] - sy;
      float dist_sq = dx * dx + dy * dy;

      if (dist_sq >= range_sq) continue;

      // Find insertion point (sorted ascending by distance)
      uint8_t insert_at = count;
      for (uint8_t k = 0; k < count; k++) {
        if (dist_sq < distances[k]) {
          insert_at = k;
          break;
        }
      }

      // Shift entries down if array not full, or drop the farthest
      if (count < MAX_RADAR_CONTACTS) {
        for (uint8_t k = count; k > insert_at; k--) {
          rd->contacts[k] = rd->contacts[k - 1];
          distances[k] = distances[k - 1];
        }
        count++;
      } else if (insert_at < MAX_RADAR_CONTACTS) {
        for (uint8_t k = MAX_RADAR_CONTACTS - 1; k > insert_at; k--) {
          rd->contacts[k] = rd->contacts[k - 1];
          distances[k] = distances[k - 1];
        }
      } else {
        continue; // farther than all stored, array full — skip
      }

      rd->contacts[insert_at].x = od->position_orientation.position_x[j];
      rd->contacts[insert_at].y = od->position_orientation.position_y[j];
      rd->contacts[insert_at].entity_type = od->type[j]._;
      distances[insert_at] = dist_sq;
    }

    rd->contact_count = count;

    // Notify parent
    messaging_send(parent_id,
                   CREATE_MESSAGE(MESSAGE_RADAR_SCAN_COMPLETE, (int32_t)count, 0));
  }
}

static void _radar_part_dispatch(entity_id_t id, message_t msg) {
  (void)id;
  switch (msg.message) {
  case MESSAGE_BROADCAST_120HZ_BEFORE_PHYSICS:
    _radar_tick();
    break;
  }
}

void radar_part_entity_initialize(void) {
  entity_manager_vtables[ENTITY_TYPE_PART_RADAR].dispatch_message = _radar_part_dispatch;
}

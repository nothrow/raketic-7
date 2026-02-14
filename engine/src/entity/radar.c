#include "platform/platform.h"
#include "platform/math.h"
#include "radar.h"
#include "entity.h"

#include <immintrin.h>

// Radar scans for all objects within range.
// Celestial bodies (planet, moon, sun) go into the nav[] array.
// Other entities (asteroid, ship, rocket, satellite) go into contacts[].
// Both arrays are sorted by distance (closest first).
// Each contact carries entity_type and entity_ordinal for identification.

static bool _is_celestial(uint8_t type) {
  switch (type) {
  case ENTITY_TYPE_PLANET:
  case ENTITY_TYPE_MOON:
  case ENTITY_TYPE_SUN:
    return true;
  default:
    return false;
  }
}

static bool _is_scannable(uint8_t type) {
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

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// Insert a contact into a sorted array (ascending distance).
// Returns updated count.
static uint8_t _insert_contact(struct radar_contact* arr, float* dist_arr,
                               uint8_t count, uint8_t max_count,
                               float d_sq, float px, float py,
                               uint16_t ordinal, uint8_t etype) {
  // Find insertion point (sorted ascending by distance)
  uint8_t insert_at = count;
  for (uint8_t k = 0; k < count; k++) {
    if (d_sq < dist_arr[k]) {
      insert_at = k;
      break;
    }
  }

  // Shift entries down if array not full, or drop the farthest
  if (count < max_count) {
    for (uint8_t k = count; k > insert_at; k--) {
      arr[k] = arr[k - 1];
      dist_arr[k] = dist_arr[k - 1];
    }
    count++;
  } else if (insert_at < max_count) {
    for (uint8_t k = max_count - 1; k > insert_at; k--) {
      arr[k] = arr[k - 1];
      dist_arr[k] = dist_arr[k - 1];
    }
  } else {
    return count; // farther than all stored, array full — skip
  }

  arr[insert_at].x = px;
  arr[insert_at].y = py;
  arr[insert_at].entity_ordinal = ordinal;
  arr[insert_at].entity_type = etype;
  dist_arr[insert_at] = d_sq;
  return count;
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

    // Collect contacts: insertion-sort by distance.
    // Tactical and nav contacts use separate arrays.
    uint8_t tac_count = 0;
    uint8_t nav_count = 0;
    float tac_distances[MAX_RADAR_CONTACTS];
    float nav_distances[MAX_NAV_CONTACTS];

    // AVX2 broadcast radar position and range
    __m256 radar_x = _mm256_set1_ps(sx);
    __m256 radar_y = _mm256_set1_ps(sy);
    __m256 range_sq_v = _mm256_set1_ps(range_sq);

    float* obj_px = od->position_orientation.position_x;
    float* obj_py = od->position_orientation.position_y;

    // SIMD broad-phase: check distance 8 objects at a time
    for (uint32_t j = 0; j < od->active; j += 8) {
      __m256 px = _mm256_load_ps(&obj_px[j]);
      __m256 py = _mm256_load_ps(&obj_py[j]);

      __m256 dx = _mm256_sub_ps(px, radar_x);
      __m256 dy = _mm256_sub_ps(py, radar_y);

      __m256 dist_sq = _mm256_add_ps(
        _mm256_mul_ps(dx, dx),
        _mm256_mul_ps(dy, dy)
      );

      __m256 in_range = _mm256_cmp_ps(dist_sq, range_sq_v, _CMP_LT_OQ);
      int mask = _mm256_movemask_ps(in_range);

      if (mask == 0) continue;  // No objects in range in this batch

      // Extract dist_sq values for scalar processing
      __declspec(align(32)) float dist_sq_arr[8];
      _mm256_store_ps(dist_sq_arr, dist_sq);

      uint32_t remaining = MIN(od->active - j, 8);
      for (uint32_t bit = 0; bit < remaining; bit++) {
        if (!(mask & (1 << bit))) continue;

        uint32_t idx = j + bit;
        if (idx == parent_idx) continue;
        if (od->health[idx] <= 0) continue;

        uint8_t etype = od->type[idx]._;
        float d_sq = dist_sq_arr[bit];

        if (_is_celestial(etype)) {
          nav_count = _insert_contact(rd->nav, nav_distances, nav_count, MAX_NAV_CONTACTS,
                                      d_sq, obj_px[idx], obj_py[idx], (uint16_t)idx, etype);
        } else if (_is_scannable(etype)) {
          tac_count = _insert_contact(rd->contacts, tac_distances, tac_count, MAX_RADAR_CONTACTS,
                                      d_sq, obj_px[idx], obj_py[idx], (uint16_t)idx, etype);
        }
      }
    }

    rd->contact_count = tac_count;
    rd->nav_count = nav_count;

    // Notify parent
    messaging_send(parent_id,
                   CREATE_MESSAGE(MESSAGE_RADAR_SCAN_COMPLETE, (int32_t)tac_count, 0));
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

const struct radar_data* radar_get_data(entity_id_t owner) {
  struct objects_data* od = entity_manager_get_objects();
  struct parts_data* pd = entity_manager_get_parts();
  uint32_t idx = GET_ORDINAL(owner);
  if (idx >= od->active) return 0;

  uint32_t start = od->parts_start_idx[idx];
  uint32_t count = od->parts_count[idx];

  for (uint32_t p = 0; p < count; p++) {
    uint32_t pi = start + p;
    if (pd->type[pi]._ == ENTITY_TYPE_PART_RADAR) {
      return (const struct radar_data*)(pd->data[pi].data);
    }
  }
  return 0;
}

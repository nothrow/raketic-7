#include "platform/platform.h"
#include "beams.h"
#include "messaging/messaging.h"
#include "collisions/polygon.h"
#include "../generated/renderer.gen.h"

#define LASER_DAMAGE 5

static struct beams_data beams_ = { 0 };

static void _beams_data_initialize(void) {
  beams_.start_x = platform_retrieve_memory(sizeof(float) * MAX_BEAMS);
  beams_.start_y = platform_retrieve_memory(sizeof(float) * MAX_BEAMS);
  beams_.end_x = platform_retrieve_memory(sizeof(float) * MAX_BEAMS);
  beams_.end_y = platform_retrieve_memory(sizeof(float) * MAX_BEAMS);
  beams_.lifetime_ticks = platform_retrieve_memory(sizeof(uint16_t) * MAX_BEAMS);
  beams_.lifetime_max = platform_retrieve_memory(sizeof(uint16_t) * MAX_BEAMS);
  beams_.active = 0;
  beams_.capacity = MAX_BEAMS;
}

// Raycast: find nearest object/part hit by beam, shorten beam, send damage messages.
// Uses radius for quick culling, then polygon-precise ray intersection.
static void _beams_raycast(float* ex, float* ey, float sx, float sy,
                           uint32_t ignore_object_idx) {
  struct objects_data* od = entity_manager_get_objects();
  struct parts_data* ptd = entity_manager_get_parts();

  float dx = *ex - sx;
  float dy = *ey - sy;
  float dir_dot = dx * dx + dy * dy;
  if (dir_dot < 0.001f) return; // degenerate beam

  float t_min = 1.0f;
  uint32_t hit_obj_idx = UINT32_MAX;
  uint32_t hit_part_idx = UINT32_MAX;

  for (uint32_t i = 0; i < od->active; i++) {
    if (i == ignore_object_idx) continue;

    // Quick radius culling
    float cx = od->position_orientation.position_x[i] - sx;
    float cy = od->position_orientation.position_y[i] - sy;
    float r = od->position_orientation.radius[i];

    float t = (cx * dx + cy * dy) / dir_dot;
    if (t < 0.0f) t = 0.0f;
    if (t > t_min) continue;

    float px = t * dx - cx;
    float py = t * dy - cy;
    float dist_sq = px * px + py * py;

    if (dist_sq > r * r) continue; // radius miss

    // Radius hit -- try precise polygon test on object hull
    uint32_t hull_count;
    const int8_t* hull_data = _generated_get_collision_hull(od->model_idx[i], &hull_count);
    if (hull_data) {
      polygon_t poly;
      polygon_transform_hull(hull_data, hull_count, od->position_orientation.position_x[i],
                             od->position_orientation.position_y[i],
                             od->position_orientation.orientation_x[i],
                             od->position_orientation.orientation_y[i], &poly);

      float t_hit;
      if (polygon_ray_intersect(&poly, sx, sy, sx + dx * t_min, sy + dy * t_min, &t_hit)) {
        float t_world = t_hit * t_min; // remap to original [0,1] range
        if (t_world < t_min) {
          t_min = t_world;
          hit_obj_idx = i;
          hit_part_idx = UINT32_MAX; // hit the object hull, not a part
        }
      }
    } else {
      // No polygon data -- fall back to radius hit
      t_min = t;
      hit_obj_idx = i;
      hit_part_idx = UINT32_MAX;
    }

    // Also check this object's parts
    uint32_t parts_start = od->parts_start_idx[i];
    uint32_t parts_count = od->parts_count[i];
    for (uint32_t p = 0; p < parts_count; p++) {
      uint32_t pi = parts_start + p;
      if (ptd->model_idx[pi] == 0xFFFF) continue;

      uint32_t part_hull_count;
      const int8_t* part_hull = _generated_get_collision_hull(ptd->model_idx[pi], &part_hull_count);
      if (!part_hull) continue;

      polygon_t part_poly;
      polygon_transform_hull(part_hull, part_hull_count, ptd->world_position_orientation.position_x[pi],
                             ptd->world_position_orientation.position_y[pi],
                             ptd->world_position_orientation.orientation_x[pi],
                             ptd->world_position_orientation.orientation_y[pi], &part_poly);

      float t_hit;
      if (polygon_ray_intersect(&part_poly, sx, sy, sx + dx * t_min, sy + dy * t_min, &t_hit)) {
        float t_world = t_hit * t_min;
        if (t_world < t_min) {
          t_min = t_world;
          hit_obj_idx = i;
          hit_part_idx = pi;
        }
      }
    }
  }

  if (hit_obj_idx != UINT32_MAX) {
    // Shorten beam to nearest hit
    *ex = sx + dx * t_min;
    *ey = sy + dy * t_min;

    // Send damage to the object
    entity_id_t target = entity_manager_resolve_object(hit_obj_idx);
    message_t msg = CREATE_MESSAGE(MESSAGE_BEAM_HIT, target._, LASER_DAMAGE);
    messaging_send(target, msg);

    // If a specific part was hit, also notify the part
    if (hit_part_idx != UINT32_MAX) {
      entity_id_t part_id = entity_manager_resolve_part(hit_part_idx);
      message_t part_msg = CREATE_MESSAGE(MESSAGE_COLLIDE_PART, target._, part_id._);
      messaging_send(part_id, part_msg);
    }
  }
}

void beams_create(float start_x, float start_y, float end_x, float end_y,
                  uint16_t lifetime, uint32_t ignore_object_idx) {
  if (beams_.active >= beams_.capacity) {
    return; // drop if full
  }

  // raycast before storing -- may shorten end point
  _beams_raycast(&end_x, &end_y, start_x, start_y, ignore_object_idx);

  uint32_t idx = beams_.active++;
  beams_.start_x[idx] = start_x;
  beams_.start_y[idx] = start_y;
  beams_.end_x[idx] = end_x;
  beams_.end_y[idx] = end_y;
  beams_.lifetime_ticks[idx] = lifetime;
  beams_.lifetime_max[idx] = lifetime;
}

struct beams_data* beams_get_data(void) {
  return &beams_;
}

static void _beams_tick(void) {
  // decrement lifetime and compact
  uint32_t write_idx = 0;
  
  for (uint32_t i = 0; i < beams_.active; i++) {
    if (beams_.lifetime_ticks[i] > 1) {
      // copy to write position (compacting)
      if (write_idx != i) {
        beams_.start_x[write_idx] = beams_.start_x[i];
        beams_.start_y[write_idx] = beams_.start_y[i];
        beams_.end_x[write_idx] = beams_.end_x[i];
        beams_.end_y[write_idx] = beams_.end_y[i];
        beams_.lifetime_max[write_idx] = beams_.lifetime_max[i];
      }
      beams_.lifetime_ticks[write_idx] = beams_.lifetime_ticks[i] - 1;
      write_idx++;
    }
  }
  
  beams_.active = write_idx;
}

static void _beams_dispatch(entity_id_t id, message_t msg) {
  (void)id;
  switch (msg.message) {
    case MESSAGE_BROADCAST_120HZ_AFTER_PHYSICS:
      _beams_tick();
      break;
  }
}

void beams_entity_initialize(void) {
  _beams_data_initialize();
  entity_manager_vtables[ENTITY_TYPE_BEAMS].dispatch_message = _beams_dispatch;
}

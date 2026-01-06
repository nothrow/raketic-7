#include "camera.h"
#include "entity.h"
#include "platform/platform.h"
#include "debug/profiler.h"

#include <immintrin.h>

#define CAMERA_LOOKAHEAD_TIME 0.3f
#define CAMERA_SMOOTHING 0.08f
#define SCREEN_CENTER_X (WINDOW_WIDTH / 2.0f)
#define SCREEN_CENTER_Y (WINDOW_HEIGHT / 2.0f)

static entity_id_t _target_entity = INVALID_ENTITY;
static double _absolute_x = 0.0;
static double _absolute_y = 0.0;
static float _smooth_offset_x = 0.0f;
static float _smooth_offset_y = 0.0f;

void camera_set_entity(entity_id_t entity_id) {
  _target_entity = entity_id;
}

void camera_get_absolute_position(double* out_x, double* out_y) {
  *out_x = _absolute_x;
  *out_y = _absolute_y;
}

static void _camera_relocate_world(void) {
  if (!is_valid_id(_target_entity)) {
    return;
  }

  PROFILE_ZONE("camera_relocate_world");

  struct objects_data* od = entity_manager_get_objects();
  uint32_t target_idx = GET_ORDINAL(_target_entity);

  _ASSERT(target_idx < od->active);

  float target_x = od->position_orientation.position_x[target_idx];
  float target_y = od->position_orientation.position_y[target_idx];
  float vel_x = od->velocity_x[target_idx];
  float vel_y = od->velocity_y[target_idx];

  float desired_offset_x = vel_x * CAMERA_LOOKAHEAD_TIME;
  float desired_offset_y = vel_y * CAMERA_LOOKAHEAD_TIME;

  _smooth_offset_x += (desired_offset_x - _smooth_offset_x) * CAMERA_SMOOTHING;
  _smooth_offset_y += (desired_offset_y - _smooth_offset_y) * CAMERA_SMOOTHING;

  float offset_x = target_x + _smooth_offset_x - SCREEN_CENTER_X;
  float offset_y = target_y + _smooth_offset_y - SCREEN_CENTER_Y;

  _absolute_x += (double)offset_x;
  _absolute_y += (double)offset_y;

  __m256 voffset_x = _mm256_set1_ps(offset_x);
  __m256 voffset_y = _mm256_set1_ps(offset_y);

  {
    float* px = od->position_orientation.position_x;
    float* py = od->position_orientation.position_y;
    float* end_px = px + od->active;

    for (; px < end_px; px += 8, py += 8) {
      __m256 pos_x = _mm256_load_ps(px);
      __m256 pos_y = _mm256_load_ps(py);
      _mm256_store_ps(px, _mm256_sub_ps(pos_x, voffset_x));
      _mm256_store_ps(py, _mm256_sub_ps(pos_y, voffset_y));
    }
  }

  {
    struct parts_data* pd = entity_manager_get_parts();
    float* px = pd->world_position_orientation.position_x;
    float* py = pd->world_position_orientation.position_y;
    float* end_px = px + pd->active;

    for (; px < end_px; px += 8, py += 8) {
      __m256 pos_x = _mm256_load_ps(px);
      __m256 pos_y = _mm256_load_ps(py);
      _mm256_store_ps(px, _mm256_sub_ps(pos_x, voffset_x));
      _mm256_store_ps(py, _mm256_sub_ps(pos_y, voffset_y));
    }
  }

  {
    struct particles_data* particles = entity_manager_get_particles();
    float* px = particles->position_orientation.position_x;
    float* py = particles->position_orientation.position_y;
    float* end_px = px + particles->active;

    for (; px < end_px; px += 8, py += 8) {
      __m256 pos_x = _mm256_load_ps(px);
      __m256 pos_y = _mm256_load_ps(py);
      _mm256_store_ps(px, _mm256_sub_ps(pos_x, voffset_x));
      _mm256_store_ps(py, _mm256_sub_ps(pos_y, voffset_y));
    }
  }

  PROFILE_ZONE_END();
}

static void _camera_dispatch(entity_id_t id, message_t msg) {
  (void)id;

  switch (msg.message) {
  case MESSAGE_BROADCAST_120HZ_AFTER_PHYSICS:
    _camera_relocate_world();
    break;
  }
}

void camera_entity_initialize(void) {
  entity_manager_vtables[ENTITY_TYPE_CAMERA].dispatch_message = _camera_dispatch;
}


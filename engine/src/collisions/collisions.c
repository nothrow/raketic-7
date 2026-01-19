#include "collisions.h"
#include "debug/profiler.h"
#include "entity/entity.h"
#include "platform/platform.h"

#include <immintrin.h>

struct collisions_engine_data {
  uint32_t active;
  uint32_t capacity;

  uint32_t* idx;
};

struct collision_buffer {
  uint32_t active;
  uint32_t capacity;

  struct {
    uint32_t idxa;
    uint32_t idxb;
  }* idx;
};

static struct collisions_engine_data culled_objects_;
static struct collisions_engine_data culled_particles_;

static struct collision_buffer collision_buffer_;

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void _cull_visible_objects(position_orientation_t* po, size_t active, struct collisions_engine_data* target) {
  float* px = po->position_x;
  float* py = po->position_y;
  float* end_px = po->position_x + active;

  target->active = 0;
  size_t remaining = active;

  // cull objects that are within camera reach (-1000, -1000) to (1000, 1000)
  __m256 minx = _mm256_set1_ps(-1000.0f);
  __m256 maxx = _mm256_set1_ps(1000.0f);
  __m256 miny = _mm256_set1_ps(-1000.0f);
  __m256 maxy = _mm256_set1_ps(1000.0f);

  for (; px < end_px; px += 8, py += 8, remaining -= 8) {
    __m256 pxv = _mm256_load_ps(px);
    __m256 pyv = _mm256_load_ps(py);

    __m256 cmpx_min = _mm256_cmp_ps(pxv, minx, _CMP_GE_OQ);
    __m256 cmpx_max = _mm256_cmp_ps(pxv, maxx, _CMP_LE_OQ);
    __m256 cmpy_min = _mm256_cmp_ps(pyv, miny, _CMP_GE_OQ);
    __m256 cmpy_max = _mm256_cmp_ps(pyv, maxy, _CMP_LE_OQ);

    __m256 cmpx = _mm256_and_ps(cmpx_min, cmpx_max);
    __m256 cmpy = _mm256_and_ps(cmpy_min, cmpy_max);
    __m256 cmp = _mm256_and_ps(cmpx, cmpy);

    int mask = _mm256_movemask_ps(cmp);
    for (size_t i = 0; i < MIN(remaining, 8); i++) {
      if (mask & (1 << i)) {
        size_t obj_idx = (px - po->position_x) + i;
        target->idx[target->active++] = (uint32_t)obj_idx;
      }
    }
  }
}

void collisions_engine_initialize(void) {
  struct objects_data* od = entity_manager_get_objects();
  struct particles_data* pd = entity_manager_get_particles();

  collision_buffer_.capacity = od->capacity;
  collision_buffer_.active = 0;
  collision_buffer_.idx = platform_retrieve_memory(sizeof(uint32_t) * 2 * collision_buffer_.capacity);

  culled_objects_.capacity = od->capacity;
  culled_objects_.active = 0;
  culled_objects_.idx = platform_retrieve_memory(sizeof(uint32_t) * culled_objects_.capacity);

  culled_particles_.capacity = pd->capacity;
  culled_particles_.active = 0;
  culled_particles_.idx = platform_retrieve_memory(sizeof(uint32_t) * culled_particles_.capacity);
}

static void _check_collisions_step(
  struct collision_buffer* collision_buffer,
  const position_orientation_t* po,
  const float* radius,
  const struct collisions_engine_data* target,
  const size_t idx) {
    __m256 px = _mm256_set1_ps(po->position_x[idx]);
    __m256 py = _mm256_set1_ps(po->position_y[idx]);
    __m256 pr = _mm256_set1_ps(radius[idx]);


    size_t remaining = target->active - ((idx + 1) & ~(size_t)0x7);
    // start aligned to 8
    // we will remove j<=idx in postprocessing
    for (size_t j = (idx + 1) & ~(size_t)0x7; j < target->active; j += 8, remaining -= 8) {
      __m256 pxj = _mm256_load_ps(&po->position_x[j]);
      __m256 pyj = _mm256_load_ps(&po->position_y[j]);

      __m256 dx = _mm256_sub_ps(px, pxj);
      __m256 dy = _mm256_sub_ps(py, pyj);

      __m256 d = _mm256_add_ps(_mm256_mul_ps(dx, dx), _mm256_mul_ps(dy, dy));
      __m256 r = _mm256_add_ps(pr, _mm256_load_ps(&radius[j]));

      __m256 cmp = _mm256_cmp_ps(d, _mm256_mul_ps(r, r), _CMP_LE_OQ);
      int mask = _mm256_movemask_ps(cmp);
      for (size_t i = 0; i < MIN(remaining, 8); i++) {
        size_t obj_idx = j + i;
        if (mask & (1 << i) && obj_idx > idx) {
          collision_buffer->idx[collision_buffer->active].idxa = (uint32_t)idx;
          collision_buffer->idx[collision_buffer->active].idxb = (uint32_t)obj_idx;
          collision_buffer->active++;
        }
      }
    }
}

void _check_collisions(struct collision_buffer* collision_buffer, const position_orientation_t* po, const float* radius, const struct collisions_engine_data* target) {
  for (size_t i = 0; i < target->active; i++) {
    _check_collisions_step(collision_buffer, po, radius, target, i);
  }
}


void collisions_engine_tick(void) {
  struct objects_data* od = entity_manager_get_objects();
  struct particles_data* pd = entity_manager_get_particles();

  PROFILE_ZONE("collisions_engine_tick");

  {
    PROFILE_ZONE("culling");
    _cull_visible_objects(&od->position_orientation, od->active, &culled_objects_);
    _cull_visible_objects(&pd->position_orientation, pd->active, &culled_particles_);
    PROFILE_ZONE_END();
  }

  PROFILE_PLOT("culled_objects", culled_objects_.active);
  PROFILE_PLOT("culled_particles", culled_particles_.active);

  collision_buffer_.active = 0;
  {
    PROFILE_ZONE("checking collisions");
    _check_collisions(&collision_buffer_, &od->position_orientation, od->radius, &culled_objects_);
    PROFILE_ZONE_END();
  }

  PROFILE_PLOT("collisions_objects", collision_buffer_.active);

  PROFILE_ZONE_END();
}


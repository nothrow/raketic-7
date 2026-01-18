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

static struct collisions_engine_data culled_objects_;
static struct collisions_engine_data culled_particles_;

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

  culled_objects_.capacity = od->capacity;
  culled_objects_.active = 0;
  culled_objects_.idx = platform_retrieve_memory(sizeof(uint32_t) * culled_objects_.capacity);

  culled_particles_.capacity = pd->capacity;
  culled_particles_.active = 0;
  culled_particles_.idx = platform_retrieve_memory(sizeof(uint32_t) * culled_particles_.capacity);
}


void collisions_engine_tick(void) {
  struct objects_data* od = entity_manager_get_objects();
  struct particles_data* pd = entity_manager_get_particles();

  PROFILE_ZONE("collisions_engine_tick");

  _cull_visible_objects(&od->position_orientation, od->active, &culled_objects_);
  _cull_visible_objects(&pd->position_orientation, pd->active, &culled_particles_);

  PROFILE_PLOT("culled_objects", culled_objects_.active);
  PROFILE_PLOT("culled_particles", culled_particles_.active);

  PROFILE_ZONE_END();
}


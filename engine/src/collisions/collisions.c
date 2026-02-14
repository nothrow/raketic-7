#include "collisions.h"
#include "radial.h"
#include "debug/profiler.h"
#include "entity/entity.h"
#include "entity/camera.h"
#include "platform/platform.h"
#include "messaging/messaging.h"
#include "../generated/renderer.gen.h"

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

static struct collision_buffer collision_buffer_objects_;
static struct collision_buffer collision_buffer_particles_;

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void _cull_visible_objects(position_orientation_t* po, size_t active, struct collisions_engine_data* target) {
  float* px = po->position_x;
  float* py = po->position_y;
  float* end_px = po->position_x + active;

  target->active = 0;
  size_t remaining = active;

  // cull objects that are within camera reach, scaled by zoom
  float zoom = camera_get_zoom();
  float half_w = (WINDOW_WIDTH / 2.0f) * zoom + 200.0f;  // margin for objects partially on-screen
  float half_h = (WINDOW_HEIGHT / 2.0f) * zoom + 200.0f;

  __m256 minx = _mm256_set1_ps(-half_w);
  __m256 maxx = _mm256_set1_ps(WINDOW_WIDTH + half_w);
  __m256 miny = _mm256_set1_ps(-half_h);
  __m256 maxy = _mm256_set1_ps(WINDOW_HEIGHT + half_h);

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

void _collision_buffer_initialize(struct collision_buffer* buffer, size_t capacity) {
  buffer->capacity = (uint32_t)capacity;
  buffer->active = 0;
  buffer->idx = platform_retrieve_memory(sizeof(uint32_t) * 2 * buffer->capacity);
}

void _collisions_engine_data_initialize(struct collisions_engine_data* data, size_t capacity) {
  data->capacity = (uint32_t)capacity;
  data->active = 0;
  data->idx = platform_retrieve_memory(sizeof(uint32_t) * data->capacity);
}

void collisions_engine_initialize(void) {
  struct objects_data* od = entity_manager_get_objects();
  struct particles_data* pd = entity_manager_get_particles();

  _collision_buffer_initialize(&collision_buffer_objects_, od->capacity);
  _collision_buffer_initialize(&collision_buffer_particles_, pd->capacity);

  _collisions_engine_data_initialize(&culled_objects_, od->capacity);
  _collisions_engine_data_initialize(&culled_particles_, pd->capacity);
}

static void
_check_collisions_step(struct collision_buffer* collision_buffer, const position_orientation_t* pa,
                       const position_orientation_t* pb, const struct collisions_engine_data* target, const size_t idx,
                       // where to start. when doing particle<->object, from=0; when doing object<->object, from=idx+1
                       const size_t from) {
  uint32_t source_obj_idx = culled_objects_.idx[idx];
  __m256 px = _mm256_set1_ps(pa->position_x[source_obj_idx]);
  __m256 py = _mm256_set1_ps(pa->position_y[source_obj_idx]);
  __m256 pr = _mm256_set1_ps(pa->radius[source_obj_idx]);


  // start aligned down to 8; skip elements before 'from' in the inner loop
  for (size_t j = from & ~(size_t)0x7; j < target->active; j += 8) {
    size_t remaining = target->active - j;

    __m256i offsets = _mm256_loadu_si256((__m256i*)&target->idx[j]);

    __m256 pxj = _mm256_i32gather_ps(pb->position_x, offsets, 4);
    __m256 pyj = _mm256_i32gather_ps(pb->position_y, offsets, 4);

    __m256 dx = _mm256_sub_ps(px, pxj);
    __m256 dy = _mm256_sub_ps(py, pyj);

    __m256 d = _mm256_add_ps(_mm256_mul_ps(dx, dx), _mm256_mul_ps(dy, dy));
    __m256 r = _mm256_add_ps(pr, _mm256_i32gather_ps(pb->radius, offsets, 4));

    __m256 cmp = _mm256_cmp_ps(d, _mm256_mul_ps(r, r), _CMP_LE_OQ);

    int mask = _mm256_movemask_ps(cmp);
    for (size_t i = 0; i < MIN(remaining, 8); i++) {

      if (mask & (1 << i) && (j + i) >= from) {
        size_t target_obj_idx = target->idx[j + i];

        collision_buffer->idx[collision_buffer->active].idxa = (uint32_t)source_obj_idx;
        collision_buffer->idx[collision_buffer->active].idxb = (uint32_t)target_obj_idx;
        collision_buffer->active++;
      }
    }
  }
}

void _check_collisions(const position_orientation_t* po, const position_orientation_t* pp) {
  for (size_t i = 0; i < culled_objects_.active; i++) {
    _check_collisions_step(&collision_buffer_objects_, po, po, &culled_objects_, i, i + 1);
    _check_collisions_step(&collision_buffer_particles_, po, pp, &culled_particles_, i, 0);
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

  collision_buffer_objects_.active = 0;
  collision_buffer_particles_.active = 0;
  {
    PROFILE_ZONE("checking collisions");
    _check_collisions(&od->position_orientation, &pd->position_orientation);
    PROFILE_ZONE_END();
  }

  PROFILE_PLOT("collisions_objects", collision_buffer_objects_.active);
  PROFILE_PLOT("collisions_particles", collision_buffer_particles_.active);

  // Narrow-phase: radial profile test to filter broad-phase false positives
  {
    PROFILE_ZONE("narrow_phase");

    uint32_t write = 0;
    for (uint32_t i = 0; i < collision_buffer_objects_.active; i++) {
      uint32_t idx_a = collision_buffer_objects_.idx[i].idxa;
      uint32_t idx_b = collision_buffer_objects_.idx[i].idxb;

      const uint8_t* prof_a = _generated_get_radial_profile(od->model_idx[idx_a]);
      const uint8_t* prof_b = _generated_get_radial_profile(od->model_idx[idx_b]);

      // If either object has no radial profile, fall back to broad-phase (keep the pair)
      if (!prof_a || !prof_b) {
        collision_buffer_objects_.idx[write++] = collision_buffer_objects_.idx[i];
        continue;
      }

      if (radial_collision_test(prof_a, od->position_orientation.position_x[idx_a],
                                od->position_orientation.position_y[idx_a],
                                od->position_orientation.orientation_x[idx_a],
                                od->position_orientation.orientation_y[idx_a], prof_b,
                                od->position_orientation.position_x[idx_b],
                                od->position_orientation.position_y[idx_b],
                                od->position_orientation.orientation_x[idx_b],
                                od->position_orientation.orientation_y[idx_b])) {
        collision_buffer_objects_.idx[write++] = collision_buffer_objects_.idx[i];
      }
    }
    collision_buffer_objects_.active = write;

    PROFILE_ZONE_END();
  }

  PROFILE_PLOT("collisions_confirmed", collision_buffer_objects_.active);

  // Dispatch object-object collision messages + part-level hit detection
  for (size_t i = 0; i < collision_buffer_objects_.active; i++) {
    uint32_t idx_a = collision_buffer_objects_.idx[i].idxa;
    uint32_t idx_b = collision_buffer_objects_.idx[i].idxb;

    // Sun: anything touching it is instantly incinerated (no explosion, no messages)
    if (od->type[idx_a]._ == ENTITY_TYPE_SUN) {
      od->health[idx_b] = 0;
      continue;
    }
    if (od->type[idx_b]._ == ENTITY_TYPE_SUN) {
      od->health[idx_a] = 0;
      continue;
    }

    entity_id_t idxa = entity_manager_resolve_object(idx_a);
    entity_id_t idxb = entity_manager_resolve_object(idx_b);

    message_t collision = CREATE_MESSAGE(MESSAGE_COLLIDE_OBJECT_OBJECT, idxa._, idxb._);
    messaging_send(idxa, collision);
    messaging_send(idxb, collision);

    // Part-level hit detection
    struct parts_data* ptd = entity_manager_get_parts();

    const uint8_t* prof_a = _generated_get_radial_profile(od->model_idx[idx_a]);
    const uint8_t* prof_b = _generated_get_radial_profile(od->model_idx[idx_b]);

    // Check parts of B hit by A's radial profile
    if (prof_a) {
      uint32_t parts_start_b = od->parts_start_idx[idx_b];
      uint32_t parts_count_b = od->parts_count[idx_b];
      for (uint32_t p = 0; p < parts_count_b; p++) {
        uint32_t pi = parts_start_b + p;
        if (ptd->model_idx[pi] == 0xFFFF)
          continue;

        const uint8_t* part_prof = _generated_get_radial_profile(ptd->model_idx[pi]);
        if (!part_prof)
          continue;

        if (radial_collision_test(prof_a, od->position_orientation.position_x[idx_a],
                                  od->position_orientation.position_y[idx_a],
                                  od->position_orientation.orientation_x[idx_a],
                                  od->position_orientation.orientation_y[idx_a], part_prof,
                                  ptd->world_position_orientation.position_x[pi],
                                  ptd->world_position_orientation.position_y[pi],
                                  ptd->world_position_orientation.orientation_x[pi],
                                  ptd->world_position_orientation.orientation_y[pi])) {
          entity_id_t part_id = entity_manager_resolve_part(pi);
          message_t part_msg = CREATE_MESSAGE(MESSAGE_COLLIDE_PART, idxa._, part_id._);
          messaging_send(part_id, part_msg);
        }
      }
    }

    // Check parts of A hit by B's radial profile
    if (prof_b) {
      uint32_t parts_start_a = od->parts_start_idx[idx_a];
      uint32_t parts_count_a = od->parts_count[idx_a];
      for (uint32_t p = 0; p < parts_count_a; p++) {
        uint32_t pi = parts_start_a + p;
        if (ptd->model_idx[pi] == 0xFFFF)
          continue;

        const uint8_t* part_prof = _generated_get_radial_profile(ptd->model_idx[pi]);
        if (!part_prof)
          continue;

        if (radial_collision_test(prof_b, od->position_orientation.position_x[idx_b],
                                  od->position_orientation.position_y[idx_b],
                                  od->position_orientation.orientation_x[idx_b],
                                  od->position_orientation.orientation_y[idx_b], part_prof,
                                  ptd->world_position_orientation.position_x[pi],
                                  ptd->world_position_orientation.position_y[pi],
                                  ptd->world_position_orientation.orientation_x[pi],
                                  ptd->world_position_orientation.orientation_y[pi])) {
          entity_id_t part_id = entity_manager_resolve_part(pi);
          message_t part_msg = CREATE_MESSAGE(MESSAGE_COLLIDE_PART, idxb._, part_id._);
          messaging_send(part_id, part_msg);
        }
      }
    }
  }

  for (size_t i = 0; i < collision_buffer_particles_.active; i++) {
    entity_id_t idxa = entity_manager_resolve_object(collision_buffer_particles_.idx[i].idxa);
    size_t particleidx = collision_buffer_particles_.idx[i].idxb;

    message_t collision = CREATE_MESSAGE(MESSAGE_COLLIDE_OBJECT_PARTICLE, idxa._, particleidx);
    messaging_send(idxa, collision);
    messaging_send(TYPE_BROADCAST(ENTITY_TYPEREF_PARTICLES), collision);
  }

  PROFILE_ZONE_END();
}

#ifdef UNIT_TESTS
#include "../test/unity.h"
#include <string.h>

// Helper to check if a collision pair exists in the buffer (order-independent)
static int _collision_exists(struct collision_buffer* buf, uint32_t a, uint32_t b) {
  for (uint32_t i = 0; i < buf->active; i++) {
    if ((buf->idx[i].idxa == a && buf->idx[i].idxb == b) ||
        (buf->idx[i].idxa == b && buf->idx[i].idxb == a)) {
      return 1;
    }
  }
  return 0;
}

// Helper to count how many times a pair appears
static int _collision_count(struct collision_buffer* buf, uint32_t a, uint32_t b) {
  int count = 0;
  for (uint32_t i = 0; i < buf->active; i++) {
    if ((buf->idx[i].idxa == a && buf->idx[i].idxb == b) ||
        (buf->idx[i].idxa == b && buf->idx[i].idxb == a)) {
      count++;
    }
  }
  return count;
}

// Test: No duplicate collision pairs
void collision_test__no_duplicates(void) {
  // Setup: 4 objects all at origin with radius 10 (all collide with each other)
  struct objects_data* od = entity_manager_get_objects();

  // Clear and setup positions - all at (0,0) so they all collide
  memset(od->position_orientation.position_x, 0, sizeof(float) * 32);
  memset(od->position_orientation.position_y, 0, sizeof(float) * 32);
  for (int i = 0; i < 16; i++) {
    od->position_orientation.radius[i] = 10.0f;
  }
  od->active = 16;

  // Manually set culled indices: 0, 1, 2, 3 (contiguous)
  culled_objects_.idx[0] = 0;
  culled_objects_.idx[1] = 1;
  culled_objects_.idx[2] = 2;
  culled_objects_.idx[3] = 3;
  culled_objects_.active = 4;

  // Pad with zeros to avoid reading garbage
  for (int i = 4; i < 16; i++) {
    culled_objects_.idx[i] = 0;
  }

  // Clear collision buffer
  collision_buffer_objects_.active = 0;

  // Run collision detection
  _check_collisions(&od->position_orientation, &od->position_orientation);

  // Expected pairs: (0,1), (0,2), (0,3), (1,2), (1,3), (2,3) = 6 pairs
  // Each should appear exactly once
  TEST_ASSERT_EQUAL_INT(1, _collision_count(&collision_buffer_objects_, 0, 1));
  TEST_ASSERT_EQUAL_INT(1, _collision_count(&collision_buffer_objects_, 0, 2));
  TEST_ASSERT_EQUAL_INT(1, _collision_count(&collision_buffer_objects_, 0, 3));
  TEST_ASSERT_EQUAL_INT(1, _collision_count(&collision_buffer_objects_, 1, 2));
  TEST_ASSERT_EQUAL_INT(1, _collision_count(&collision_buffer_objects_, 1, 3));
  TEST_ASSERT_EQUAL_INT(1, _collision_count(&collision_buffer_objects_, 2, 3));

  // Total should be exactly 6
  TEST_ASSERT_EQUAL_UINT32(6, collision_buffer_objects_.active);
}

// Test: Gather works correctly with scattered/non-contiguous indices
void collision_test__scattered_indices(void) {
  struct objects_data* od = entity_manager_get_objects();

  // Setup: Objects at scattered positions
  // Object 0: (0, 0)
  // Object 5: (1, 0)   <- close to 0, should collide
  // Object 10: (100, 100) <- far away
  // Object 15: (2, 0)  <- close to 0 and 5, should collide with both
  memset(od->position_orientation.position_x, 0, sizeof(float) * 64);
  memset(od->position_orientation.position_y, 0, sizeof(float) * 64);

  od->position_orientation.position_x[0] = 0.0f;
  od->position_orientation.position_y[0] = 0.0f;
  od->position_orientation.radius[0] = 5.0f;

  od->position_orientation.position_x[5] = 1.0f;
  od->position_orientation.position_y[5] = 0.0f;
  od->position_orientation.radius[5] = 5.0f;

  od->position_orientation.position_x[10] = 100.0f;
  od->position_orientation.position_y[10] = 100.0f;
  od->position_orientation.radius[10] = 5.0f;

  od->position_orientation.position_x[15] = 2.0f;
  od->position_orientation.position_y[15] = 0.0f;
  od->position_orientation.radius[15] = 5.0f;

  od->active = 24;  // Must be multiple of 8

  // Set scattered culled indices (holes in the array)
  culled_objects_.idx[0] = 0;
  culled_objects_.idx[1] = 5;
  culled_objects_.idx[2] = 10;
  culled_objects_.idx[3] = 15;
  culled_objects_.active = 4;

  // Pad to avoid garbage reads
  for (int i = 4; i < 16; i++) {
    culled_objects_.idx[i] = 0;
  }

  collision_buffer_objects_.active = 0;

  _check_collisions(&od->position_orientation, &od->position_orientation);

  // Expected collisions:
  // - 0 and 5: distance=1, radii=10 -> collide
  // - 0 and 15: distance=2, radii=10 -> collide
  // - 5 and 15: distance=1, radii=10 -> collide
  // - 10 is far from everyone -> no collisions
  TEST_ASSERT_TRUE(_collision_exists(&collision_buffer_objects_, 0, 5));
  TEST_ASSERT_TRUE(_collision_exists(&collision_buffer_objects_, 0, 15));
  TEST_ASSERT_TRUE(_collision_exists(&collision_buffer_objects_, 5, 15));

  // Object 10 should not collide with anyone
  TEST_ASSERT_FALSE(_collision_exists(&collision_buffer_objects_, 0, 10));
  TEST_ASSERT_FALSE(_collision_exists(&collision_buffer_objects_, 5, 10));
  TEST_ASSERT_FALSE(_collision_exists(&collision_buffer_objects_, 10, 15));

  TEST_ASSERT_EQUAL_UINT32(3, collision_buffer_objects_.active);
}

// Test: Don't read beyond active count
void collision_test__respects_active_count(void) {
  struct objects_data* od = entity_manager_get_objects();

  // Setup: Put colliding objects beyond the active count
  // These should NOT be detected
  memset(od->position_orientation.position_x, 0, sizeof(float) * 64);
  memset(od->position_orientation.position_y, 0, sizeof(float) * 64);

  // Object 0 and 1: both at origin, will collide
  od->position_orientation.position_x[0] = 0.0f;
  od->position_orientation.position_y[0] = 0.0f;
  od->position_orientation.radius[0] = 10.0f;

  od->position_orientation.position_x[1] = 0.0f;
  od->position_orientation.position_y[1] = 0.0f;
  od->position_orientation.radius[1] = 10.0f;

  // Object 2: also at origin (would collide if included)
  od->position_orientation.position_x[2] = 0.0f;
  od->position_orientation.position_y[2] = 0.0f;
  od->position_orientation.radius[2] = 10.0f;

  od->active = 24;

  // Only include objects 0 and 1 in culled array
  culled_objects_.idx[0] = 0;
  culled_objects_.idx[1] = 1;
  culled_objects_.active = 2;  // Only 2 objects!

  // Put object 2's index beyond active (should be ignored)
  culled_objects_.idx[2] = 2;

  // Pad rest
  for (int i = 3; i < 16; i++) {
    culled_objects_.idx[i] = 0;
  }

  collision_buffer_objects_.active = 0;

  _check_collisions(&od->position_orientation, &od->position_orientation);

  // Should only detect collision between 0 and 1
  TEST_ASSERT_EQUAL_UINT32(1, collision_buffer_objects_.active);
  TEST_ASSERT_TRUE(_collision_exists(&collision_buffer_objects_, 0, 1));

  // Object 2 should NOT appear in any collisions
  TEST_ASSERT_FALSE(_collision_exists(&collision_buffer_objects_, 0, 2));
  TEST_ASSERT_FALSE(_collision_exists(&collision_buffer_objects_, 1, 2));
}

// Test: Edge case - exactly 8 objects (aligned)
void collision_test__aligned_8_objects(void) {
  struct objects_data* od = entity_manager_get_objects();

  // All 8 objects at origin - all collide
  for (int i = 0; i < 8; i++) {
    od->position_orientation.position_x[i] = 0.0f;
    od->position_orientation.position_y[i] = 0.0f;
    od->position_orientation.radius[i] = 10.0f;
    culled_objects_.idx[i] = i;
  }
  od->active = 8;
  culled_objects_.active = 8;

  // Pad
  for (int i = 8; i < 16; i++) {
    culled_objects_.idx[i] = 0;
  }

  collision_buffer_objects_.active = 0;

  _check_collisions(&od->position_orientation, &od->position_orientation);

  // 8 choose 2 = 28 pairs
  TEST_ASSERT_EQUAL_UINT32(28, collision_buffer_objects_.active);
}

// Test: Edge case - 9 objects (not aligned, tests boundary)
void collision_test__unaligned_9_objects(void) {
  struct objects_data* od = entity_manager_get_objects();

  // 9 objects at origin - all collide
  for (int i = 0; i < 16; i++) {
    od->position_orientation.position_x[i] = 0.0f;
    od->position_orientation.position_y[i] = 0.0f;
    od->position_orientation.radius[i] = 10.0f;
    culled_objects_.idx[i] = i;
  }
  od->active = 16;
  culled_objects_.active = 9;  // Only 9 objects

  collision_buffer_objects_.active = 0;

  _check_collisions(&od->position_orientation, &od->position_orientation);

  // 9 choose 2 = 36 pairs
  TEST_ASSERT_EQUAL_UINT32(36, collision_buffer_objects_.active);
}

#endif

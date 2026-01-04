#include "physics.h"
#include "entity/entity.h"

#include <immintrin.h>

#define YOSHIDA_C1 0.6756035959798289f
#define YOSHIDA_C2 -0.1756035959798288f
#define YOSHIDA_C3 -0.1756035959798288f
#define YOSHIDA_C4 0.6756035959798289f

#define YOSHIDA_D1 1.3512071919596578f
#define YOSHIDA_D2 -1.7024143839193155f
#define YOSHIDA_D3 1.3512071919596578f

static void _particle_manager_euler(struct particles_data* pd) {
  __m256 dt = _mm256_set1_ps(TICK_S);

  float* px = pd->position_orientation.position_x;
  float* py = pd->position_orientation.position_y;

  float* vx = pd->velocity_x;
  float* vy = pd->velocity_y;

  float* end_pos = pd->position_orientation.position_x + pd->active;

  // no need for testing velocities range, they are the same size as positions
  for (; px < end_pos; px += 8, py += 8, vx += 8, vy += 8) {
    __m256 pxv = _mm256_load_ps(px);
    __m256 pyv = _mm256_load_ps(py);
    __m256 vvx = _mm256_load_ps(vx);
    __m256 vyv = _mm256_load_ps(vy);

    __m256 pxn = _mm256_fmadd_ps(vvx, dt, pxv);
    __m256 pyn = _mm256_fmadd_ps(vyv, dt, pyv);

    _mm256_store_ps(px, pxn);
    _mm256_store_ps(py, pyn);
  }
}

static void _objects_apply_yoshida(struct objects_data* od) {
  (od);
  // Yoshida integrator implementation would go here
  // For simplicity, this is left as a placeholder
}


static uint32_t _particle_manager_ttl(struct particles_data* pd) {
  __m256i zero = _mm256_setzero_si256();
  __m256i one = _mm256_set1_epi16(1);

  uint16_t* lifetime_ticks = pd->lifetime_ticks;
  uint16_t* end_life = pd->lifetime_ticks + pd->active;

  uint32_t alive_count = 0;

  for (; lifetime_ticks < end_life; lifetime_ticks += 16) {
    __m256i lifetime = _mm256_load_si256((const __m256i*)lifetime_ticks);
    lifetime = _mm256_subs_epu16(lifetime, one);
    _mm256_store_si256((__m256i*)lifetime_ticks, lifetime);

    // Accumulate: 0xFFFF for alive, 0x0000 for dead
    __m256i alive_mask = _mm256_cmpgt_epi16(lifetime, zero);
    alive_count += _mm_popcnt_u32(_mm256_movemask_epi8(alive_mask)) / 2; // /2 because 16-bit elements, 8-bit mask
  }

  return alive_count;
}

static void _parts_world_transform(struct objects_data* od, struct parts_data* pd) {
  for (uint32_t i = 0; i < pd->active; i += 8) {
    uint32_t parent_idx = GET_ORDINAL(pd->parent_id[i]);

    _ASSERT(parent_idx >= 0 && parent_idx <= od->active);
    _ASSERT(od->parts_start_idx[parent_idx] <= i && od->parts_start_idx[parent_idx] + od->parts_count[parent_idx] >= i);

    __m256 parent_x = _mm256_set1_ps(od->position_orientation.position_x[parent_idx]);
    __m256 parent_y = _mm256_set1_ps(od->position_orientation.position_y[parent_idx]);

    __m256 parent_ox = _mm256_set1_ps(od->position_orientation.orientation_x[parent_idx]);
    __m256 parent_oy = _mm256_set1_ps(od->position_orientation.orientation_y[parent_idx]);

    __m256 local_x = _mm256_load_ps(&pd->local_offset_x[i]);
    __m256 local_y = _mm256_load_ps(&pd->local_offset_y[i]);

    // rotate local offset by parent's orientation
    __m256 rotated_lx = _mm256_fmadd_ps(local_x, parent_ox, _mm256_mul_ps(local_y, parent_oy));
    __m256 rotated_ly = _mm256_fmsub_ps(local_y, parent_ox, _mm256_mul_ps(local_x, parent_oy));

    // set world position
    __m256 world_px = _mm256_add_ps(parent_x, rotated_lx);
    __m256 world_py = _mm256_add_ps(parent_y, rotated_ly);
    _mm256_store_ps(&pd->world_position_orientation.position_x[i], world_px);
    _mm256_store_ps(&pd->world_position_orientation.position_y[i], world_py);

    __m256 local_ox = _mm256_load_ps(&pd->local_orientation_x[i]);
    __m256 local_oy = _mm256_load_ps(&pd->local_orientation_y[i]);
    // rotate local orientation by parent's orientation
    __m256 world_ox = _mm256_fmsub_ps(local_ox, parent_ox, _mm256_mul_ps(local_oy, parent_oy));
    __m256 world_oy = _mm256_fmadd_ps(local_ox, parent_oy, _mm256_mul_ps(local_oy, parent_ox));

    // normalize world orientation
    __m256 length_sq =
        _mm256_rsqrt_ps(_mm256_add_ps(_mm256_mul_ps(world_ox, world_ox), _mm256_mul_ps(world_oy, world_oy)));
    world_ox = _mm256_mul_ps(world_ox, length_sq);
    world_oy = _mm256_mul_ps(world_oy, length_sq);
    _mm256_store_ps(&pd->world_position_orientation.orientation_x[i], world_ox);
    _mm256_store_ps(&pd->world_position_orientation.orientation_y[i], world_oy);
  }
}

static void _objects_tick(void) {
  struct objects_data* od = entity_manager_get_objects();
  struct parts_data* pd = entity_manager_get_parts();

  _objects_apply_yoshida(od);
  _parts_world_transform(od, pd);
}

static void _particle_manager_tick(void) {
  struct particles_data* pd = entity_manager_get_particles();

  _particle_manager_euler(pd);
  if (_particle_manager_ttl(pd) < pd->active) {
    entity_manager_pack_particles();
  }
}

void physics_engine_tick(void) {
  _objects_tick();
  _particle_manager_tick();

  messaging_send(RECIPIENT_ID_BROADCAST, CREATE_MESSAGE(MESSAGE_BROADCAST_120HZ_TICK, 0, 0));
}

#ifdef UNIT_TESTS
#include "entity/types.h"
#include "../test/unity.h"

void physics_test__parts_world_transform_rotations(void) {
  struct objects_data* od = entity_manager_get_objects();
  struct parts_data* pd = entity_manager_get_parts();

  od->active = 2;
  od->position_orientation.orientation_x[0] = 0.0f;
  od->position_orientation.orientation_y[0] = 1.0f;

  od->position_orientation.orientation_x[1] = 1.0f;
  od->position_orientation.orientation_y[1] = 0.0f;

  od->parts_start_idx[0] = 0;
  od->parts_count[0] = 2;

  od->parts_start_idx[1] = 8;
  od->parts_count[1] = 2;


  pd->active = 16;
  for (int i = 0; i < 2; i++)
  {
    od->parts_start_idx[i] = 8*i;
    od->parts_count[i] = 2;

    pd->parent_id[8 * i] = OBJECT_ID_WITH_TYPE(i, ENTITY_TYPE_ANY);

    pd->local_orientation_x[8 * i + 0] = 1.0f;
    pd->local_orientation_y[8 * i + 0] = 0.0f;

    pd->local_orientation_x[8 * i + 1] = 0.0f;
    pd->local_orientation_y[8 * i + 1] = 1.0f;
  }


  _parts_world_transform(od, pd);

  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, pd->world_position_orientation.orientation_x[0]);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, pd->world_position_orientation.orientation_y[0]);

  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, pd->world_position_orientation.orientation_x[1]);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, pd->world_position_orientation.orientation_y[1]);

  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, pd->world_position_orientation.orientation_x[8]);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, pd->world_position_orientation.orientation_y[8]);

  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, pd->world_position_orientation.orientation_x[9]);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, pd->world_position_orientation.orientation_y[9]);
}
#endif

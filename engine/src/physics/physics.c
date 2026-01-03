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

static void _objects_tick(void) {
  struct objects_data* od = entity_manager_get_objects();

  _objects_apply_yoshida(od);
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

  messaging_send(RECIPIENT_TYPE_ANY, RECIPIENT_ID_BROADCAST, CREATE_MESSAGE(MESSAGE_BROADCAST_120HZ_TICK, 0, 0));
}

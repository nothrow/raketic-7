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

  float* positions = (float*)pd->position;
  float* velocities = (float*)pd->velocity;

  float* end_pos = (float*)(pd->position + pd->active);

  // no need for testing velocities range, they are the same size as positions
  for (; positions < end_pos; positions += 8, velocities += 8) {
    __m256 pos = _mm256_load_ps(positions);
    __m256 vel = _mm256_load_ps(velocities);

    pos = _mm256_fmadd_ps(vel, dt, pos);

    _mm256_store_ps(positions, pos);
  }
}

static void _objects_apply_yoshida(struct objects_data* od) {
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
}

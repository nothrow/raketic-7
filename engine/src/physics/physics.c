#include "physics.h"
#include "entity/entity.h"

#include <immintrin.h>

static void particle_manager_euler(struct particles_data* pd) {
  __m256d dt = _mm256_set1_pd(TICK_S);

  double* positions = (double*)pd->position;
  double* velocities = (double*)pd->velocity;

  double* end_pos = (double*)(pd->position + pd->active);

  // no need for testing velocities range, they are the same size as positions
  for (; positions < end_pos; positions += 4, velocities += 4) {
    __m256d pos = _mm256_load_pd(positions);
    __m256d vel = _mm256_load_pd(velocities);

    pos = _mm256_fmadd_pd(vel, dt, pos);

    _mm256_store_pd(positions, pos);
  }
}

static uint32_t particle_manager_ttl(struct particles_data* pd) {
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

static void particle_manager_tick(void) {
  struct particles_data* pd = entity_manager_get_particles();

  particle_manager_euler(pd);
  if (particle_manager_ttl(pd) < pd->active) {
    entity_manager_pack_particles();
  }
}

void physics_engine_tick(void) {
  particle_manager_tick();
}

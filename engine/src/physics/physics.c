#include "physics.h"
#include "platform/platform.h"
#include "entity/entity.h"

#include <immintrin.h>

static void particle_manager_euler(struct particles_data* pd)
{
  __m256d dt = _mm256_set1_pd(TICK_S);

  double* positions = (double*)pd->position;
  double* velocities = (double*)pd->velocity;

  double* end_pos = (double*)(pd->position + pd->active);

  // no need for testing velocities range, they are the same size as positions
  for (; positions < end_pos; positions += 4, velocities += 4)
  {
    __m256d pos = _mm256_load_pd(positions);
    __m256d vel = _mm256_load_pd(velocities);

    pos = _mm256_fmadd_pd(vel, dt, pos);

    _mm256_store_pd(positions, pos);
  }
}

static void particle_manager_ttl(struct particles_data* pd)
{
  __m256i one = _mm256_set1_epi16(1);

  uint16_t* lifetime_ticks = pd->lifetime_ticks;
  uint16_t* end_life = pd->lifetime_ticks + pd->active;

  for (;
    lifetime_ticks < end_life;
    lifetime_ticks += 16)
  {
    __m256i lifetime = _mm256_load_si256((const __m256i*)lifetime_ticks);
    lifetime = _mm256_subs_epu16(lifetime, one);
    _mm256_store_si256((__m256i*)lifetime_ticks, lifetime);
  }
}

static void particle_manager_tick(void)
{
  struct particles_data* pd = entity_manager_get_particles();

  particle_manager_euler(pd);
  particle_manager_ttl(pd);
}


void physics_engine_tick(void)
{
  particle_manager_tick();
}

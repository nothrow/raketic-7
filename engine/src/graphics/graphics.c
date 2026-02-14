#include "graphics.h"
#include "stars.h"
#include "entity/entity.h"
#include "entity/camera.h"
#include "entity/beams.h"
#include "platform/platform.h"
#include "debug/profiler.h"

#include <immintrin.h>

// Expose zoom state for external use (debug overlays, etc.)
static float _current_zoom = 1.0f;

static void _particles_colors(const struct particles_data* pd, color_t* target) {
  PROFILE_ZONE("_particles_colors");
  uint16_t* __restrict lifetime_ticks = pd->lifetime_ticks;
  uint16_t* __restrict lifetime_max = pd->lifetime_max;

  uint16_t* end_life = pd->lifetime_ticks + pd->active;

  __m256 zero = _mm256_setzero_ps();
  __m256 three = _mm256_set1_ps(3.0f);
  __m256 one = _mm256_set1_ps(1.0f);
  __m256 two = _mm256_set1_ps(2.0f);
  __m256 bytemax = _mm256_set1_ps(255.0f);

  __m256i shuffle_pattern = _mm256_setr_epi8(0, 4, 8, 12,  // r0 g0 b0 a0
                                             1, 5, 9, 13,  // r1 g1 b1 a1
                                             2, 6, 10, 14, // r2 g2 b2 a2
                                             3, 7, 11, 15, // r3 g3 b3 a3
                                             // high lane (same pattern)
                                             0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15);

  for (; lifetime_ticks < end_life; lifetime_ticks += 8, lifetime_max += 8, target += 8) {
    __m128i life = _mm_loadu_si128((const __m128i*)lifetime_ticks); // l0, l1, l2, l3, l4, l5, l6, l7
    __m128i max = _mm_loadu_si128((const __m128i*)lifetime_max);    // m0, m1, m2, m3, m4, m5, m6, m7

    // extend to unsigned 32b first
    __m256i life32 = _mm256_cvtepu16_epi32(life); // l0, l1, l2, l3, l4, l5, l6, l7
    __m256i max32 = _mm256_cvtepu16_epi32(max);   // m0, m1, m2, m3, m4, m5, m6, m7

    // convert to floats then (there is no uint16 -> float)
    __m256 lifef = _mm256_cvtepi32_ps(life32); // l0, l1, l2, l3, l4, l5, l6, l7
    __m256 maxf = _mm256_cvtepi32_ps(max32);   // m0, m1, m2, m3, m4, m5, m6, m7

    // avoid division by zero
    __m256 maxf_safe = _mm256_max_ps(maxf, _mm256_set1_ps(1.0f));

    // have particle life ratio now
    __m256 t = _mm256_div_ps(lifef, maxf_safe); // r0, r1, r2, r3, r4, r5, r6, r7

    // R = min(1, t*3), G = min(1, max(0, t*3-1)), B = max(0, t*3-2) (nice, computation friendly, fire-like colors)
    __m256 t3 = _mm256_mul_ps(t, three);
    __m256 r = _mm256_min_ps(one, t3);

    __m256 t3_minus_1 = _mm256_sub_ps(t3, one);
    __m256 g = _mm256_min_ps(one, _mm256_max_ps(zero, t3_minus_1));

    __m256 t3_minus_2 = _mm256_sub_ps(t3, two);
    __m256 b = _mm256_max_ps(zero, t3_minus_2);

    __m256i ri = _mm256_cvtps_epi32(_mm256_mul_ps(r, bytemax));
    __m256i gi = _mm256_cvtps_epi32(_mm256_mul_ps(g, bytemax));
    __m256i bi = _mm256_cvtps_epi32(_mm256_mul_ps(b, bytemax));

    // now pack!
    __m256i rgba_8 = _mm256_packus_epi16(
        _mm256_packus_epi32(ri, gi),
        _mm256_packus_epi32(bi, _mm256_setzero_si256())); // r0 r1 r2 r3 g0 g1 g2 g3 b0 b1 b2 b3 0 0 0 0 r4 r5 r6 r7 g4
                                                          // g5 g6 g7 b4 b5 b6 b7 0 0 0 0

    __m256i rgba_shuffled = _mm256_shuffle_epi8(rgba_8, shuffle_pattern);
    // r0 g0 b0 0 r1 g1 b1 0 r2 g2 b2 0 r3 g3 b3 0 r4 g4 b4 0 r5 g5 b5 0 r6 g6 b6 0 r7 g7 b7 0
    _mm256_storeu_si256((__m256i*)(target), rgba_shuffled);
  }
  PROFILE_ZONE_END();
}

static void _graphics_particles_draw(void) {
  PROFILE_ZONE("_graphics_particles_draw");
  struct particles_data* pd = entity_manager_get_particles();
  // compute colors!
  color_t* colors = (color_t*)pd->temporary;

  _particles_colors(pd, colors);

  platform_renderer_draw_models(pd->active, colors, &pd->position_orientation, pd->model_idx);
  PROFILE_ZONE_END();
}

static void _graphics_objects_draw(void) {
  PROFILE_ZONE("_graphics_objects_draw");
  struct objects_data* od = entity_manager_get_objects();

  platform_renderer_draw_models(od->active, NULL, &od->position_orientation, od->model_idx);
  PROFILE_ZONE_END();
}

static void _graphics_parts_draw(void) {
  PROFILE_ZONE("_graphics_parts_draw");
  struct parts_data* pd = entity_manager_get_parts();

  platform_renderer_draw_models(pd->active, NULL, &pd->world_position_orientation, pd->model_idx);
  PROFILE_ZONE_END();
}

static void _graphics_beams_draw(void) {
  PROFILE_ZONE("_graphics_beams_draw");
  struct beams_data* bd = beams_get_data();
  
  if (bd->active > 0) {
    platform_renderer_draw_beams(bd->active, bd->start_x, bd->start_y,
                                 bd->end_x, bd->end_y,
                                 bd->lifetime_ticks, bd->lifetime_max);
  }
  PROFILE_ZONE_END();
}

void graphics_initialize(void) {
  stars_initialize();
}

void graphics_engine_draw(void) {
  PROFILE_ZONE("graphics_engine_draw");

  double cam_x, cam_y;
  camera_get_absolute_position(&cam_x, &cam_y);
  _current_zoom = camera_get_zoom();

  // Draw star background first (screen-space, no zoom)
  stars_draw((float)cam_x, (float)cam_y);

  // World rendering with zoom
  platform_renderer_push_zoom(_current_zoom);

  _graphics_particles_draw();
  _graphics_beams_draw();
  _graphics_parts_draw();
  _graphics_objects_draw();

  platform_renderer_pop_zoom();

  PROFILE_ZONE_END();
}

#include "explosion.h"
#include "particles.h"
#include "platform/math.h"
#include "../generated/renderer.gen.h"

#define SPARK_BASE_COUNT   15
#define SPARK_PER_INTENSITY 0.5f
#define SPARK_BASE_SPEED   80.0f
#define SPARK_SPEED_VARIANCE 0.6f
#define SPARK_TTL_MIN      0.2f
#define SPARK_TTL_MAX      0.5f

#define SHRAPNEL_BASE_COUNT   3
#define SHRAPNEL_PER_INTENSITY 0.1f
#define SHRAPNEL_BASE_SPEED   40.0f
#define SHRAPNEL_SPEED_VARIANCE 0.5f
#define SHRAPNEL_TTL_MIN      0.4f
#define SHRAPNEL_TTL_MAX      1.0f

#define MAX_EXPLOSION_PARTICLES 80

void explosion_spawn(float x, float y, float vx, float vy, float intensity) {
  // clamp intensity for particle count calculation
  if (intensity < 0.0f) intensity = 0.0f;

  // --- Sparks: fast, short-lived, small ---
  int spark_count = (int)(SPARK_BASE_COUNT + intensity * SPARK_PER_INTENSITY);
  if (spark_count > MAX_EXPLOSION_PARTICLES) spark_count = MAX_EXPLOSION_PARTICLES;

  for (int i = 0; i < spark_count; i++) {
    // random direction (full 360 degrees)
    float dx = randf_symmetric();
    float dy = randf_symmetric();

    // avoid degenerate zero-length vectors
    if (dx == 0.0f && dy == 0.0f) dx = 1.0f;

    // normalize roughly
    float len = dx * dx + dy * dy;
    if (len > 0.01f) {
      float inv = 1.0f / (len > 1.0f ? len : 1.0f); // cheap normalize (intentionally imperfect for variation)
      dx *= inv;
      dy *= inv;
    }

    float speed = SPARK_BASE_SPEED * (1.0f - SPARK_SPEED_VARIANCE + randf() * SPARK_SPEED_VARIANCE * 2.0f);

    float ttl_f = SPARK_TTL_MIN + randf() * (SPARK_TTL_MAX - SPARK_TTL_MIN);
    uint16_t ttl = (uint16_t)(ttl_f * TICKS_IN_SECOND);

    particle_create_t pc = {
      .x = x + dx * 2.0f, // slight offset from center
      .y = y + dy * 2.0f,
      .vx = vx * 0.3f + dx * speed, // inherit some parent velocity
      .vy = vy * 0.3f + dy * speed,
      .ox = dx,
      .oy = dy,
      .ttl = ttl,
      .model_idx = MODEL_SPARK_IDX
    };

    particles_create_particle(&pc);
  }

  // --- Shrapnel: slower, larger, longer-lived ---
  int shrapnel_count = (int)(SHRAPNEL_BASE_COUNT + intensity * SHRAPNEL_PER_INTENSITY);
  if (shrapnel_count > 20) shrapnel_count = 20;

  for (int i = 0; i < shrapnel_count; i++) {
    float dx = randf_symmetric();
    float dy = randf_symmetric();

    if (dx == 0.0f && dy == 0.0f) dx = 1.0f;

    float len = dx * dx + dy * dy;
    if (len > 0.01f) {
      float inv = 1.0f / (len > 1.0f ? len : 1.0f);
      dx *= inv;
      dy *= inv;
    }

    float speed = SHRAPNEL_BASE_SPEED * (1.0f - SHRAPNEL_SPEED_VARIANCE + randf() * SHRAPNEL_SPEED_VARIANCE * 2.0f);

    float ttl_f = SHRAPNEL_TTL_MIN + randf() * (SHRAPNEL_TTL_MAX - SHRAPNEL_TTL_MIN);
    uint16_t ttl = (uint16_t)(ttl_f * TICKS_IN_SECOND);

    particle_create_t pc = {
      .x = x + dx * 3.0f,
      .y = y + dy * 3.0f,
      .vx = vx * 0.5f + dx * speed,
      .vy = vy * 0.5f + dy * speed,
      .ox = randf_symmetric(), // random tumble orientation
      .oy = randf_symmetric(),
      .ttl = ttl,
      .model_idx = MODEL_SHRAPNEL_IDX
    };

    particles_create_particle(&pc);
  }
}

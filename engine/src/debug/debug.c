#include "debug.h"
#include "debug_font.h"
#include "entity/entity.h"
#include "entity/camera.h"
#include "collisions/radial.h"
#include "../generated/renderer.gen.h"
#include "platform/platform.h"

#include <math.h>

static entity_id_t watch_target_ = INVALID_ENTITY;
static bool debug_overlay_enabled_ = true;

static const color_t COLOR_WHITE = { 255, 255, 255, 255 };
static const color_t COLOR_GREEN = { 100, 255, 100, 255 };
static const color_t COLOR_RED = { 255, 100, 100, 255 };
static const color_t COLOR_YELLOW = { 255, 255, 100, 255 };
static const color_t COLOR_CYAN = { 100, 255, 255, 255 };

#define ARROW_SIZE 5.0f
#define VECTOR_SCALE 20.0f
#define TEXT_SCALE 1.2f
#define LINE_HEIGHT 14.0f

static void draw_arrow(float x1, float y1, float x2, float y2, color_t color) {
  platform_debug_draw_line(x1, y1, x2, y2, color);

  float dx = x2 - x1;
  float dy = y2 - y1;
  float len = dx * dx + dy * dy;
  if (len < 1.0f)
    return;

  len = 1.0f / (float)sqrtf(len);
  dx *= len;
  dy *= len;

  float px = -dy;
  float py = dx;

  float ax1 = x2 - dx * ARROW_SIZE + px * ARROW_SIZE * 0.5f;
  float ay1 = y2 - dy * ARROW_SIZE + py * ARROW_SIZE * 0.5f;
  float ax2 = x2 - dx * ARROW_SIZE - px * ARROW_SIZE * 0.5f;
  float ay2 = y2 - dy * ARROW_SIZE - py * ARROW_SIZE * 0.5f;

  platform_debug_draw_line(x2, y2, ax1, ay1, color);
  platform_debug_draw_line(x2, y2, ax2, ay2, color);
}

static void draw_vector(float ox, float oy, float vx, float vy, float scale, color_t color) {
  float ex = ox + vx * scale;
  float ey = oy + vy * scale;
  draw_arrow(ox, oy, ex, ey, color);
}

void debug_initialize(void) {
  watch_target_ = (entity_id_t)INVALID_ENTITY;
  debug_overlay_enabled_ = true;
}

void debug_toggle_overlay(void) {
  debug_overlay_enabled_ = !debug_overlay_enabled_;
}

bool debug_is_overlay_enabled(void) {
  return debug_overlay_enabled_;
}

void debug_watch_set(entity_id_t id) {
  watch_target_ = id;
}

void debug_watch_draw(void) {
  if (!is_valid_id(watch_target_))
    return;
  if (IS_PART(watch_target_))
    return;

  struct objects_data* od = entity_manager_get_objects();
  struct parts_data* pd = entity_manager_get_parts();

  uint32_t idx = GET_ORDINAL(watch_target_);
  if (idx >= od->active)
    return;

  float px = od->position_orientation.position_x[idx];
  float py = od->position_orientation.position_y[idx];
  float vx = od->velocity_x[idx];
  float vy = od->velocity_y[idx];
  float ox = od->position_orientation.orientation_x[idx];
  float oy = od->position_orientation.orientation_y[idx];
  float ax = od->acceleration_x[idx];
  float ay = od->acceleration_y[idx];
  float thrust = od->thrust[idx];

  draw_vector(px, py, vx, vy, 1.0f, COLOR_GREEN);
  draw_vector(px, py, ox, oy, VECTOR_SCALE, COLOR_WHITE);
  if (thrust > 0.001f) {
    draw_vector(px, py, ox * thrust * 0.1f, oy * thrust * 0.1f, VECTOR_SCALE, COLOR_RED);
  }

  float tx = px + 60.0f;
  float ty = py - 40.0f;

  debug_font_draw_string(tx, ty, "pos:", TEXT_SCALE, COLOR_YELLOW);
  debug_font_draw_float(tx + 30, ty, px, 1, TEXT_SCALE, COLOR_WHITE);
  debug_font_draw_float(tx + 80, ty, py, 1, TEXT_SCALE, COLOR_WHITE);
  ty += LINE_HEIGHT;

  debug_font_draw_string(tx, ty, "vel:", TEXT_SCALE, COLOR_YELLOW);
  debug_font_draw_float(tx + 30, ty, vx, 2, TEXT_SCALE, COLOR_GREEN);
  debug_font_draw_float(tx + 80, ty, vy, 2, TEXT_SCALE, COLOR_GREEN);
  ty += LINE_HEIGHT;

  debug_font_draw_string(tx, ty, "ori:", TEXT_SCALE, COLOR_YELLOW);
  debug_font_draw_float(tx + 30, ty, ox, 2, TEXT_SCALE, COLOR_WHITE);
  debug_font_draw_float(tx + 80, ty, oy, 2, TEXT_SCALE, COLOR_WHITE);
  ty += LINE_HEIGHT;

  debug_font_draw_string(tx, ty, "acc:", TEXT_SCALE, COLOR_YELLOW);
  debug_font_draw_float(tx + 30, ty, ax, 2, TEXT_SCALE, COLOR_WHITE);
  debug_font_draw_float(tx + 80, ty, ay, 2, TEXT_SCALE, COLOR_WHITE);
  ty += LINE_HEIGHT;

  debug_font_draw_string(tx, ty, "thr:", TEXT_SCALE, COLOR_YELLOW);
  debug_font_draw_float(tx + 30, ty, thrust, 1, TEXT_SCALE, COLOR_RED);
  ty += LINE_HEIGHT * 1.5f;

  uint32_t parts_start = od->parts_start_idx[idx];
  uint32_t parts_count = od->parts_count[idx];

  for (uint32_t p = 0; p < parts_count; p++) {
    uint32_t pi = parts_start + p;
    if (pi >= pd->active)
      break;

    float plx = pd->local_offset_x[pi];
    float ply = pd->local_offset_y[pi];
    float wpx = pd->world_position_orientation.position_x[pi];
    float wpy = pd->world_position_orientation.position_y[pi];
    float wox = pd->world_position_orientation.orientation_x[pi];
    float woy = pd->world_position_orientation.orientation_y[pi];

    draw_vector(wpx, wpy, wox * 0.5f, woy * 0.5f, VECTOR_SCALE, COLOR_CYAN);

    debug_font_draw_string(tx, ty, "part", TEXT_SCALE, COLOR_CYAN);
    debug_font_draw_float(tx + 30, ty, (float)p, 0, TEXT_SCALE, COLOR_CYAN);
    ty += LINE_HEIGHT;

    debug_font_draw_string(tx + 10, ty, "loc:", TEXT_SCALE, COLOR_YELLOW);
    debug_font_draw_float(tx + 40, ty, plx, 1, TEXT_SCALE, COLOR_WHITE);
    debug_font_draw_float(tx + 90, ty, ply, 1, TEXT_SCALE, COLOR_WHITE);
    ty += LINE_HEIGHT;

    debug_font_draw_string(tx + 10, ty, "abs:", TEXT_SCALE, COLOR_YELLOW);
    debug_font_draw_float(tx + 40, ty, wpx, 1, TEXT_SCALE, COLOR_WHITE);
    debug_font_draw_float(tx + 90, ty, wpy, 1, TEXT_SCALE, COLOR_WHITE);
    ty += LINE_HEIGHT;
  }
}

// --- Trail System ---
// Positions in objects_data are camera-relative (camera always near origin).
// We store camera-relative positions AND the camera's absolute position at each
// sample, so when drawing we can compensate for camera movement:
//   draw_pos = recorded_pos + (recorded_cam - current_cam)
//
// Compaction-safe: we track the entity type at each trail slot. If the entity
// type changes (due to pack_objects reshuffling), the trail for that slot resets.
// Transient entities (rockets) are skipped entirely.

#define TRAIL_MAX_OBJECTS 16
#define TRAIL_POINTS 1024
#define TRAIL_SAMPLE_EVERY 12
#define TRAIL_PALETTE_COUNT 8

static float trail_x_[TRAIL_MAX_OBJECTS][TRAIL_POINTS];
static float trail_y_[TRAIL_MAX_OBJECTS][TRAIL_POINTS];
static double trail_cam_x_[TRAIL_POINTS];
static double trail_cam_y_[TRAIL_POINTS];
static uint32_t trail_head_ = 0;
static uint32_t trail_tick_ = 0;

// Per-object: valid sample count and entity type for change detection
static uint32_t trail_obj_count_[TRAIL_MAX_OBJECTS];
static uint8_t trail_obj_type_[TRAIL_MAX_OBJECTS];

static const color_t trail_palette_[TRAIL_PALETTE_COUNT] = {
    {100, 255, 100, 255}, // green
    {255, 255, 100, 255}, // yellow
    {100, 200, 255, 255}, // light blue
    {255, 100, 255, 255}, // magenta
    {255, 180, 50, 255},  // orange
    {100, 255, 255, 255}, // cyan
    {200, 200, 200, 255}, // grey
    {255, 100, 100, 255}, // red
};

static bool _trail_is_transient(uint8_t type) {
  return type == ENTITY_TYPE_ROCKET;
}

void debug_trails_draw(void) {
  struct objects_data* od = entity_manager_get_objects();
  uint32_t obj_count = od->active < TRAIL_MAX_OBJECTS ? od->active : TRAIL_MAX_OBJECTS;

  double cam_x, cam_y;
  camera_get_absolute_position(&cam_x, &cam_y);

  // Record current positions at interval
  if (++trail_tick_ % TRAIL_SAMPLE_EVERY == 0) {
    trail_cam_x_[trail_head_] = cam_x;
    trail_cam_y_[trail_head_] = cam_y;

    for (uint32_t i = 0; i < obj_count; i++) {
      uint8_t type = od->type[i]._;

      // Skip transient entities (rockets etc.)
      if (_trail_is_transient(type))
        continue;

      // Detect entity change at this index (compaction reshuffles objects)
      if (trail_obj_type_[i] != type) {
        trail_obj_count_[i] = 0;
        trail_obj_type_[i] = type;
      }

      trail_x_[i][trail_head_] = od->position_orientation.position_x[i];
      trail_y_[i][trail_head_] = od->position_orientation.position_y[i];

      if (trail_obj_count_[i] < TRAIL_POINTS)
        trail_obj_count_[i]++;
    }

    trail_head_ = (trail_head_ + 1) % TRAIL_POINTS;
  }

  // Draw trail lines
  for (uint32_t i = 0; i < obj_count; i++) {
    uint32_t count = trail_obj_count_[i];
    if (count < 2)
      continue;

    if (_trail_is_transient(od->type[i]._))
      continue;

    color_t base = trail_palette_[i % TRAIL_PALETTE_COUNT];

    for (uint32_t j = 1; j < count; j++) {
      uint32_t p0 = (trail_head_ + TRAIL_POINTS - count + j - 1) % TRAIL_POINTS;
      uint32_t p1 = (trail_head_ + TRAIL_POINTS - count + j) % TRAIL_POINTS;

      // Compensate for camera movement since each sample was recorded
      float x0 = trail_x_[i][p0] + (float)(trail_cam_x_[p0] - cam_x);
      float y0 = trail_y_[i][p0] + (float)(trail_cam_y_[p0] - cam_y);
      float x1 = trail_x_[i][p1] + (float)(trail_cam_x_[p1] - cam_x);
      float y1 = trail_y_[i][p1] + (float)(trail_cam_y_[p1] - cam_y);

      // Fade: older segments more transparent, newer brighter
      uint8_t alpha = (uint8_t)(20 + (j * 180) / count);
      color_t c = {base.r, base.g, base.b, alpha};

      platform_debug_draw_line(x0, y0, x1, y1, c);
    }
  }
}

// --- Orbit Zones ---

#define ORBIT_CIRCLE_SEGMENTS 48
#define ORBIT_MIN_DISTANCE_FACTOR 1.5f
#define ORBIT_MAX_DISTANCE_FACTOR 3.0f
#define ORBIT_GRAVITATIONAL_CONSTANT 6.67430f
#define ORBIT_TANGENTIAL_THRESHOLD 0.6f

static const color_t COLOR_ORBIT_INNER = { 255, 80, 80, 100 };
static const color_t COLOR_ORBIT_OUTER = { 80, 160, 255, 80 };
static const color_t COLOR_ORBIT_DIAG = { 200, 200, 200, 180 };
static const color_t COLOR_ORBIT_OK = { 100, 255, 100, 200 };
static const color_t COLOR_ORBIT_FAIL = { 255, 100, 100, 200 };

static void _draw_circle(float cx, float cy, float radius, color_t color, bool dashed) {
  float step = 2.0f * 3.14159265f / ORBIT_CIRCLE_SEGMENTS;
  float prev_x = cx + radius;
  float prev_y = cy;

  for (int i = 1; i <= ORBIT_CIRCLE_SEGMENTS; i++) {
    float angle = step * i;
    float next_x = cx + radius * cosf(angle);
    float next_y = cy + radius * sinf(angle);

    if (!dashed || (i % 2 == 0)) {
      platform_debug_draw_line(prev_x, prev_y, next_x, next_y, color);
    }

    prev_x = next_x;
    prev_y = next_y;
  }
}

void debug_draw_orbit_zones(void) {
  struct objects_data* od = entity_manager_get_objects();

  for (uint32_t i = 0; i < od->active; i++) {
    if (od->type[i]._ != ENTITY_TYPE_PLANET) continue;

    float px = od->position_orientation.position_x[i];
    float py = od->position_orientation.position_y[i];
    float planet_radius = od->position_orientation.radius[i];
    float planet_mass = od->mass[i];

    _draw_circle(px, py, planet_radius * ORBIT_MIN_DISTANCE_FACTOR, COLOR_ORBIT_INNER, true);
    _draw_circle(px, py, planet_radius * ORBIT_MAX_DISTANCE_FACTOR, COLOR_ORBIT_OUTER, true);

    // Find nearest ship and show diagnostic
    for (uint32_t s = 0; s < od->active; s++) {
      if (od->type[s]._ != ENTITY_TYPE_SHIP) continue;

      float sx = od->position_orientation.position_x[s];
      float sy = od->position_orientation.position_y[s];
      float dx = sx - px;
      float dy = sy - py;
      float dist = sqrtf(dx * dx + dy * dy);

      // Only show diagnostics within a reasonable range
      if (dist > planet_radius * ORBIT_MAX_DISTANCE_FACTOR * 2.0f) continue;

      // Velocity RELATIVE to the body (body itself is moving, e.g. planet orbiting sun)
      float rel_vx = od->velocity_x[s] - od->velocity_x[i];
      float rel_vy = od->velocity_y[s] - od->velocity_y[i];
      float speed = sqrtf(rel_vx * rel_vx + rel_vy * rel_vy);

      float v_orbit = sqrtf(ORBIT_GRAVITATIONAL_CONSTANT * planet_mass / dist);
      float v_escape = sqrtf(2.0f * ORBIT_GRAVITATIONAL_CONSTANT * planet_mass / dist);

      // Radial/tangential decomposition
      float rnx = dx / (dist > 0.01f ? dist : 1.0f);
      float rny = dy / (dist > 0.01f ? dist : 1.0f);
      float v_radial = rel_vx * rnx + rel_vy * rny;
      float tang_ratio = speed > 0.01f ? fabsf(v_radial) / speed : 0.0f;

      bool in_zone = dist >= planet_radius * ORBIT_MIN_DISTANCE_FACTOR &&
                     dist <= planet_radius * ORBIT_MAX_DISTANCE_FACTOR;
      bool speed_ok = speed <= v_escape;
      bool tang_ok = tang_ratio <= ORBIT_TANGENTIAL_THRESHOLD;
      bool thrust_off = od->thrust[s] <= 0.0f;

      // Draw diagnostic to the left of the ship (right side is used by watch_draw)
      float tx = sx - 200.0f;
      float ty = sy - 40.0f;

      debug_font_draw_string(tx, ty, "ORBIT", TEXT_SCALE, COLOR_ORBIT_DIAG);
      ty += LINE_HEIGHT;

      debug_font_draw_string(tx, ty, "dist:", TEXT_SCALE, COLOR_ORBIT_DIAG);
      debug_font_draw_float(tx + 40, ty, dist, 0, TEXT_SCALE, in_zone ? COLOR_ORBIT_OK : COLOR_ORBIT_FAIL);
      debug_font_draw_string(tx + 80, ty, "/", TEXT_SCALE, COLOR_ORBIT_DIAG);
      debug_font_draw_float(tx + 90, ty, planet_radius * ORBIT_MIN_DISTANCE_FACTOR, 0, TEXT_SCALE, COLOR_ORBIT_DIAG);
      debug_font_draw_string(tx + 120, ty, "-", TEXT_SCALE, COLOR_ORBIT_DIAG);
      debug_font_draw_float(tx + 130, ty, planet_radius * ORBIT_MAX_DISTANCE_FACTOR, 0, TEXT_SCALE, COLOR_ORBIT_DIAG);
      ty += LINE_HEIGHT;

      debug_font_draw_string(tx, ty, "spd:", TEXT_SCALE, COLOR_ORBIT_DIAG);
      debug_font_draw_float(tx + 40, ty, speed, 1, TEXT_SCALE, speed_ok ? COLOR_ORBIT_OK : COLOR_ORBIT_FAIL);
      debug_font_draw_string(tx + 80, ty, "< esc", TEXT_SCALE, COLOR_ORBIT_DIAG);
      debug_font_draw_float(tx + 120, ty, v_escape, 1, TEXT_SCALE, COLOR_ORBIT_DIAG);
      ty += LINE_HEIGHT;

      debug_font_draw_string(tx, ty, "vorb:", TEXT_SCALE, COLOR_ORBIT_DIAG);
      debug_font_draw_float(tx + 40, ty, v_orbit, 1, TEXT_SCALE, COLOR_CYAN);
      ty += LINE_HEIGHT;

      debug_font_draw_string(tx, ty, "tang:", TEXT_SCALE, COLOR_ORBIT_DIAG);
      debug_font_draw_float(tx + 40, ty, tang_ratio, 2, TEXT_SCALE, tang_ok ? COLOR_ORBIT_OK : COLOR_ORBIT_FAIL);
      debug_font_draw_string(tx + 80, ty, "< 0.6", TEXT_SCALE, COLOR_ORBIT_DIAG);
      ty += LINE_HEIGHT;

      debug_font_draw_string(tx, ty, "thr:", TEXT_SCALE, COLOR_ORBIT_DIAG);
      debug_font_draw_string(tx + 40, ty, thrust_off ? "OFF" : "ON", TEXT_SCALE, thrust_off ? COLOR_ORBIT_OK : COLOR_ORBIT_FAIL);
    }
  }
}

// --- Collision Hulls ---

static const color_t COLOR_HULL_OBJECT = { 0, 255, 0, 120 };
static const color_t COLOR_HULL_PART = { 0, 200, 255, 120 };

static void _draw_radial_outline(const uint16_t* profile, float cx, float cy, float ox, float oy, color_t color) {
  float px[16], py[16];
  radial_reconstruct(profile, cx, cy, ox, oy, px, py);
  for (int i = 0; i < 16; i++) {
    int j = (i + 1) & 15;
    platform_debug_draw_line(px[i], py[i], px[j], py[j], color);
  }
}

void debug_draw_collision_hulls(void) {
  struct objects_data* od = entity_manager_get_objects();
  struct parts_data* pd = entity_manager_get_parts();

  for (uint32_t i = 0; i < od->active; i++) {
    const uint16_t* prof = _generated_get_radial_profile(od->model_idx[i]);
    if (!prof)
      continue;

    _draw_radial_outline(prof, od->position_orientation.position_x[i],
                         od->position_orientation.position_y[i],
                         od->position_orientation.orientation_x[i],
                         od->position_orientation.orientation_y[i], COLOR_HULL_OBJECT);

    // Also draw parts' radial profiles
    uint32_t parts_start = od->parts_start_idx[i];
    uint32_t parts_count = od->parts_count[i];
    for (uint32_t p = 0; p < parts_count; p++) {
      uint32_t pi = parts_start + p;
      if (pd->model_idx[pi] == 0xFFFF)
        continue;

      const uint16_t* part_prof = _generated_get_radial_profile(pd->model_idx[pi]);
      if (!part_prof)
        continue;

      _draw_radial_outline(part_prof, pd->world_position_orientation.position_x[pi],
                           pd->world_position_orientation.position_y[pi],
                           pd->world_position_orientation.orientation_x[pi],
                           pd->world_position_orientation.orientation_y[pi], COLOR_HULL_PART);
    }
  }
}

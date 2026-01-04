#include "debug.h"
#include "debug_font.h"
#include "entity/entity.h"
#include "platform/platform.h"

#include <math.h>

static entity_id_t watch_target_ = INVALID_ENTITY;

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

static void draw_vector(float ox, float oy, float vx, float vy, color_t color) {
  float ex = ox + vx * VECTOR_SCALE;
  float ey = oy + vy * VECTOR_SCALE;
  draw_arrow(ox, oy, ex, ey, color);
}

void debug_initialize(void) {
  watch_target_ = (entity_id_t)INVALID_ENTITY;
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
  float thrust = od->thrust[idx];

  draw_vector(px, py, vx, vy, COLOR_GREEN);
  draw_vector(px, py, ox, oy, COLOR_WHITE);
  if (thrust > 0.001f) {
    draw_vector(px, py, ox * thrust * 0.1f, oy * thrust * 0.1f, COLOR_RED);
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

    draw_vector(wpx, wpy, wox * 0.5f, woy * 0.5f, COLOR_CYAN);

    debug_font_draw_string(tx, ty, "part", TEXT_SCALE, COLOR_CYAN);
    debug_font_draw_float(tx + 30, ty, (float)p, 0, TEXT_SCALE, COLOR_CYAN);
    ty += LINE_HEIGHT;

    debug_font_draw_string(tx + 10, ty, "loc:", TEXT_SCALE, COLOR_YELLOW);
    debug_font_draw_float(tx + 40, ty, plx, 1, TEXT_SCALE, COLOR_WHITE);
    debug_font_draw_float(tx + 90, ty, ply, 1, TEXT_SCALE, COLOR_WHITE);
    ty += LINE_HEIGHT;
  }
}

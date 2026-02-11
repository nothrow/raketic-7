#include "hud.h"
#include "hud_text.h"
#include "entity/entity.h"
#include "entity/ai.h"
#include "platform/platform.h"
#include "platform/math.h"

#include <math.h>  // sqrtf

// ============================================================================
// Layout
// ============================================================================

#define HUD_GAUGE_PANEL_H  112.0f
#define HUD_AP_PANEL_H     148.0f
#define HUD_PANEL_MARGIN_Y 8.0f
#define HUD_GAUGE_PANEL_W  130.0f
#define HUD_AP_PANEL_W     160.0f
#define HUD_PANEL_GAP      16.0f

#define HUD_GAUGE_RADIUS     34.0f
#define HUD_GAUGE_CY_OFFSET  42.0f

// Arc spans 270 degrees: lower-left (135 deg) clockwise to lower-right (45 deg)
#define ARC_START_DEG  135
#define ARC_SWEEP_DEG  270

#define ARC_SEGMENTS   40
#define TICK_COUNT     10
#define TICK_LEN       5.0f
#define NEEDLE_LEN     28.0f

#define SPEED_MAX      60.0f
#define HEALTH_MAX     100.0f

#define TEXT_SCALE      1.0f
#define LABEL_SCALE     0.9f
#define SMALL_SCALE     0.8f

// ============================================================================
// Palette
// ============================================================================

static const color_t COL_FRAME   = { 120, 140, 160, 200 };
static const color_t COL_ACCENT  = { 160, 180, 200, 220 };
static const color_t COL_ARC     = { 80, 100, 120, 160 };
static const color_t COL_TICK    = { 150, 170, 190, 200 };
static const color_t COL_NEEDLE  = { 255, 255, 255, 255 };
static const color_t COL_TEXT    = { 200, 220, 240, 255 };
static const color_t COL_LABEL   = { 140, 160, 180, 220 };
static const color_t COL_GREEN   = { 100, 255, 100, 255 };
static const color_t COL_YELLOW  = { 255, 255, 100, 255 };
static const color_t COL_RED     = { 255, 80, 80, 255 };
static const color_t COL_CYAN    = { 100, 220, 255, 255 };
static const color_t COL_DIM     = { 80, 90, 100, 140 };
static const color_t COL_AP_COAST = { 100, 200, 255, 255 };
static const color_t COL_AP_BURN  = { 255, 160, 60, 255 };

// ============================================================================
// State
// ============================================================================

static entity_id_t hud_target_ = INVALID_ENTITY;

// ============================================================================
// Drawing primitives
// ============================================================================

static void _line(float x1, float y1, float x2, float y2, color_t c) {
  platform_renderer_draw_line(x1, y1, x2, y2, c);
}

static void _rect(float x, float y, float w, float h, color_t c) {
  _line(x, y, x + w, y, c);
  _line(x + w, y, x + w, y + h, c);
  _line(x + w, y + h, x, y + h, c);
  _line(x, y + h, x, y, c);
}

static void _corner_accents(float x, float y, float w, float h, float a, color_t c) {
  _line(x, y + a, x + a, y, c);
  _line(x + w - a, y, x + w, y + a, c);
  _line(x, y + h - a, x + a, y + h, c);
  _line(x + w - a, y + h, x + w, y + h - a, c);
}

static void _arc(float cx, float cy, float r, int start_deg, int sweep_deg, int segs, color_t c) {
  float px = cx + r * lut_cos(start_deg);
  float py = cy + r * lut_sin(start_deg);
  for (int i = 1; i <= segs; i++) {
    int a = start_deg + sweep_deg * i / segs;
    float nx = cx + r * lut_cos(a);
    float ny = cy + r * lut_sin(a);
    _line(px, py, nx, ny, c);
    px = nx;
    py = ny;
  }
}

static void _ticks(float cx, float cy, float r, float len, int count,
                   int start_deg, int sweep_deg, color_t c) {
  for (int i = 0; i <= count; i++) {
    int a = start_deg + sweep_deg * i / count;
    float ca = lut_cos(a), sa = lut_sin(a);
    _line(cx + (r - len) * ca, cy + (r - len) * sa,
          cx + r * ca, cy + r * sa, c);
  }
}

static void _needle(float cx, float cy, float len, int angle_deg, color_t c) {
  _line(cx, cy, cx + len * lut_cos(angle_deg), cy + len * lut_sin(angle_deg), c);
  // Pivot dot
  _line(cx - 1, cy, cx + 1, cy, c);
  _line(cx, cy - 1, cx, cy + 1, c);
}

// ============================================================================
// Color helpers
// ============================================================================

static color_t _color_health(float frac) {
  if (frac > 0.6f) return COL_GREEN;
  if (frac > 0.3f) return COL_YELLOW;
  return COL_RED;
}

static color_t _color_speed(float frac) {
  if (frac < 0.7f) return COL_GREEN;
  if (frac < 0.9f) return COL_YELLOW;
  return COL_RED;
}

// ============================================================================
// Int-to-string helper (no sprintf dependency)
// ============================================================================

static void _itoa(int val, char* buf, int* len) {
  int i = 0;
  if (val < 0) { buf[i++] = '-'; val = -val; }
  if (val == 0) { buf[i++] = '0'; }
  else {
    int digits[10], nd = 0;
    while (val > 0) { digits[nd++] = val % 10; val /= 10; }
    for (int d = nd - 1; d >= 0; d--) buf[i++] = (char)('0' + digits[d]);
  }
  buf[i] = 0;
  *len = i;
}

// ============================================================================
// Gauge panel (speed / health)
// ============================================================================

static void _draw_gauge(float px, float py, float pw,
                        float value, float max_val, const char* label,
                        color_t (*color_fn)(float)) {
  float cx = px + pw * 0.5f;
  float cy = py + HUD_GAUGE_CY_OFFSET;

  // Panel frame + corner accents
  _rect(px, py, pw, HUD_GAUGE_PANEL_H, COL_FRAME);
  _corner_accents(px, py, pw, HUD_GAUGE_PANEL_H, 5.0f, COL_ACCENT);

  // Arc background
  _arc(cx, cy, HUD_GAUGE_RADIUS, ARC_START_DEG, ARC_SWEEP_DEG, ARC_SEGMENTS, COL_ARC);

  // Tick marks
  _ticks(cx, cy, HUD_GAUGE_RADIUS, TICK_LEN, TICK_COUNT, ARC_START_DEG, ARC_SWEEP_DEG, COL_TICK);

  // Clamp fraction
  float frac = value / max_val;
  if (frac < 0.0f) frac = 0.0f;
  if (frac > 1.0f) frac = 1.0f;

  color_t val_col = color_fn(frac);

  // Filled arc showing current value
  if (frac > 0.01f) {
    int segs = (int)(ARC_SEGMENTS * frac) + 1;
    int fill_sweep = (int)(ARC_SWEEP_DEG * frac);
    _arc(cx, cy, HUD_GAUGE_RADIUS - 2.0f, ARC_START_DEG, fill_sweep, segs, val_col);
  }

  // Needle
  int needle_a = ARC_START_DEG + (int)(ARC_SWEEP_DEG * frac);
  _needle(cx, cy, NEEDLE_LEN, needle_a, COL_NEEDLE);

  // Numeric readout
  char buf[16];
  int len;
  _itoa((int)(value + 0.5f), buf, &len);
  float tw = hud_text_string_width(buf, TEXT_SCALE);
  float text_y = cy + HUD_GAUGE_RADIUS + 10.0f;
  hud_text_draw_string(cx - tw * 0.5f, text_y, buf, TEXT_SCALE, COL_TEXT);

  // Label
  float lw = hud_text_string_width(label, LABEL_SCALE);
  hud_text_draw_string(cx - lw * 0.5f, text_y + 13.0f, label, LABEL_SCALE, COL_LABEL);
}

// ============================================================================
// Orbit proximity computation
// ============================================================================

// Must match the constants in ship.c for orbit detection
#define GRAV_CONST             6.67430f
#define ORBIT_MIN_DIST_FACTOR  1.5f
#define ORBIT_MAX_DIST_FACTOR  3.0f
#define ORBIT_TANG_THRESHOLD   0.6f
#define NAV_DISPLAY_FACTOR     2.0f   // show mini-map within outer_zone * this

typedef struct {
  bool found;
  uint8_t body_type;
  float distance;
  float zone_inner;
  float zone_outer;
  float body_radius;
  float rel_x, rel_y;    // ship position relative to body
  float vel_x, vel_y;    // ship velocity
  bool in_nav_range;      // close enough to show the mini-map
  bool in_zone;
  bool speed_ok;
  bool tangent_ok;
  bool thrust_off;
} orbit_proximity_t;

static void _compute_orbit_proximity(struct objects_data* od, uint32_t ship_idx,
                                     orbit_proximity_t* out) {
  out->found = false;
  out->in_nav_range = false;
  out->in_zone = false;
  out->speed_ok = false;
  out->tangent_ok = false;
  out->thrust_off = false;

  if (ship_idx >= od->active) return;

  float sx = od->position_orientation.position_x[ship_idx];
  float sy = od->position_orientation.position_y[ship_idx];
  float svx = od->velocity_x[ship_idx];
  float svy = od->velocity_y[ship_idx];

  out->vel_x = svx;
  out->vel_y = svy;

  // Find nearest planet or moon
  float best_dist_sq = 1e30f;
  uint32_t best = UINT32_MAX;

  for (uint32_t i = 0; i < od->active; i++) {
    uint8_t t = od->type[i]._;
    if (t != ENTITY_TYPE_PLANET && t != ENTITY_TYPE_MOON) continue;

    float dx = od->position_orientation.position_x[i] - sx;
    float dy = od->position_orientation.position_y[i] - sy;
    float d2 = dx * dx + dy * dy;
    if (d2 < best_dist_sq) {
      best_dist_sq = d2;
      best = i;
    }
  }

  if (best == UINT32_MAX) return;

  out->found = true;
  out->body_type = od->type[best]._;

  float bx = od->position_orientation.position_x[best];
  float by = od->position_orientation.position_y[best];

  // Ship position relative to body (ship - body)
  out->rel_x = sx - bx;
  out->rel_y = sy - by;

  float dist = sqrtf(best_dist_sq);
  out->distance = dist;

  float body_radius = od->position_orientation.radius[best];
  out->body_radius = body_radius;
  out->zone_inner = body_radius * ORBIT_MIN_DIST_FACTOR;
  out->zone_outer = body_radius * ORBIT_MAX_DIST_FACTOR;

  // Nav range: show the mini-map when within 2x the outer orbit zone
  out->in_nav_range = dist <= out->zone_outer * NAV_DISPLAY_FACTOR;

  out->in_zone = dist >= out->zone_inner && dist <= out->zone_outer;

  float speed_sq = svx * svx + svy * svy;
  float speed = sqrtf(speed_sq);

  float body_mass = od->mass[best];
  float v_escape_sq = 2.0f * GRAV_CONST * body_mass / (dist > 1.0f ? dist : 1.0f);
  out->speed_ok = speed_sq <= v_escape_sq;

  if (speed > 0.01f) {
    float rnx = out->rel_x / (dist > 0.01f ? dist : 1.0f);
    float rny = out->rel_y / (dist > 0.01f ? dist : 1.0f);
    float v_radial = svx * rnx + svy * rny;
    float tang_ratio = fabsf(v_radial) / speed;
    out->tangent_ok = tang_ratio <= ORBIT_TANG_THRESHOLD;
  } else {
    out->tangent_ok = true;
  }

  out->thrust_off = od->thrust[ship_idx] <= 0.0f;
}

// ============================================================================
// Dashed circle for mini-map zone rings
// ============================================================================

#define DASHED_SEGMENTS 24

static void _draw_dashed_circle(float cx, float cy, float r, color_t c) {
  int step = 360 / DASHED_SEGMENTS;  // 15 degrees per segment
  for (int i = 0; i < DASHED_SEGMENTS; i += 2) {
    int a0 = step * i;
    int a1 = step * (i + 1);
    _line(cx + r * lut_cos(a0), cy + r * lut_sin(a0),
          cx + r * lut_cos(a1), cy + r * lut_sin(a1), c);
  }
}

// ============================================================================
// Condition indicator helper
// ============================================================================

static void _draw_condition(float x, float y, const char* label, bool ok) {
  hud_text_draw_string(x, y, label, SMALL_SCALE, COL_LABEL);
  float lw = hud_text_string_width(label, SMALL_SCALE);
  hud_text_draw_string(x + lw + 3.0f, y, ok ? "OK" : "--", SMALL_SCALE, ok ? COL_GREEN : COL_RED);
}

// ============================================================================
// Navigation / AI panel
// ============================================================================

static void _draw_nav_panel(float px, float py, float ph, float pw,
                            entity_id_t ship_id) {
  float cx = px + pw * 0.5f;
  float lx = px + 8.0f;

  // Panel frame + corner accents
  _rect(px, py, pw, ph, COL_FRAME);
  _corner_accents(px, py, pw, ph, 5.0f, COL_ACCENT);

  float ty = py + 8.0f;
  float line_h = 12.0f;

  // Title
  {
    float tw = hud_text_string_width("NAV", LABEL_SCALE);
    hud_text_draw_string(cx - tw * 0.5f, ty, "NAV", LABEL_SCALE, COL_LABEL);
  }
  ty += line_h + 3.0f;

  // --- Compute proximity ---
  struct objects_data* od = entity_manager_get_objects();
  uint32_t ship_idx = GET_ORDINAL(ship_id);

  orbit_proximity_t prox;
  _compute_orbit_proximity(od, ship_idx, &prox);

  if (!prox.found || !prox.in_nav_range) {
    // Nothing nearby — blank panel
    hud_text_draw_string(lx, ty + 20.0f, "---", SMALL_SCALE, COL_DIM);
    return;
  }

  // --- Body name + distance (top line) ---
  const char* body_name = prox.body_type == ENTITY_TYPE_PLANET ? "PLANET"
                        : prox.body_type == ENTITY_TYPE_MOON   ? "MOON"
                                                                : "BODY";
  hud_text_draw_string(lx, ty, body_name, SMALL_SCALE, COL_CYAN);
  hud_text_draw_string(lx + 42.0f, ty, "d:", SMALL_SCALE, COL_LABEL);
  hud_text_draw_float(lx + 56.0f, ty, prox.distance, 0, SMALL_SCALE, COL_TEXT);
  ty += line_h + 2.0f;

  // === Mini-map ===
  // Map shows the planet at center, with orbit zone rings and the ship dot.
  // Scale: the map viewport covers outer_zone * NAV_DISPLAY_FACTOR in world units.
  float map_cx = cx;
  float map_cy = py + 78.0f;
  float map_r = 34.0f;  // map circle radius in pixels

  float world_extent = prox.zone_outer * NAV_DISPLAY_FACTOR;
  float map_scale = map_r / world_extent;

  // Map boundary circle (dim)
  _arc(map_cx, map_cy, map_r, 0, 360, 32, COL_ARC);

  // Planet circle (solid, proportional)
  float planet_r_px = prox.body_radius * map_scale;
  if (planet_r_px < 2.0f) planet_r_px = 2.0f;
  _arc(map_cx, map_cy, planet_r_px, 0, 360, 16, COL_CYAN);

  // Inner orbit ring (dashed)
  float inner_r_px = prox.zone_inner * map_scale;
  _draw_dashed_circle(map_cx, map_cy, inner_r_px, COL_YELLOW);

  // Outer orbit ring (dashed)
  float outer_r_px = prox.zone_outer * map_scale;
  _draw_dashed_circle(map_cx, map_cy, outer_r_px, COL_GREEN);

  // Ship dot
  float ship_map_x = map_cx + prox.rel_x * map_scale;
  float ship_map_y = map_cy + prox.rel_y * map_scale;

  // Clamp ship dot to map bounds (with small margin)
  {
    float sdx = ship_map_x - map_cx;
    float sdy = ship_map_y - map_cy;
    float sd = sqrtf(sdx * sdx + sdy * sdy);
    if (sd > map_r - 2.0f) {
      float s = (map_r - 2.0f) / sd;
      ship_map_x = map_cx + sdx * s;
      ship_map_y = map_cy + sdy * s;
    }
  }

  // Ship cross marker
  color_t ship_col = prox.in_zone ? COL_GREEN : COL_TEXT;
  _line(ship_map_x - 3, ship_map_y, ship_map_x + 3, ship_map_y, ship_col);
  _line(ship_map_x, ship_map_y - 3, ship_map_x, ship_map_y + 3, ship_col);

  // Velocity vector (small arrow from ship dot)
  {
    float vlen = sqrtf(prox.vel_x * prox.vel_x + prox.vel_y * prox.vel_y);
    if (vlen > 0.1f) {
      float vnx = prox.vel_x / vlen;
      float vny = prox.vel_y / vlen;
      float arrow_len = 10.0f;
      float ax = ship_map_x + vnx * arrow_len;
      float ay = ship_map_y + vny * arrow_len;
      _line(ship_map_x, ship_map_y, ax, ay, COL_GREEN);
    }
  }

  // === Info below the map ===
  ty = map_cy + map_r + 5.0f;

  // Separator
  _line(px + 6.0f, ty, px + pw - 6.0f, ty, COL_ARC);
  ty += 4.0f;

  // Check AI orbit state
  ai_status_t ap;
  ai_query_status(ship_id, &ap);

  if (ap.active) {
    // AP active: show phase + dV
    const char* phase_str = ap.phase == 1 ? "BURN" : "COAST";
    color_t phase_col = ap.phase == 1 ? COL_AP_BURN : COL_AP_COAST;
    hud_text_draw_string(lx, ty, "AP", SMALL_SCALE, COL_LABEL);
    hud_text_draw_string(lx + 18.0f, ty, phase_str, SMALL_SCALE, phase_col);

    float dv_col_frac = ap.delta_v / 20.0f;
    if (dv_col_frac > 1.0f) dv_col_frac = 1.0f;
    color_t dv_col = dv_col_frac < 0.3f ? COL_GREEN
                   : dv_col_frac < 0.7f ? COL_YELLOW
                                         : COL_RED;
    hud_text_draw_string(cx + 4.0f, ty, "dV", SMALL_SCALE, COL_LABEL);
    hud_text_draw_float(cx + 22.0f, ty, ap.delta_v, 1, SMALL_SCALE, dv_col);

  } else if (prox.in_zone) {
    // In orbit zone: show conditions
    float rx = cx + 4.0f;
    _draw_condition(lx, ty, "zone", prox.in_zone);
    _draw_condition(rx, ty, "spd", prox.speed_ok);
    ty += line_h;
    _draw_condition(lx, ty, "tang", prox.tangent_ok);
    _draw_condition(rx, ty, "thr", prox.thrust_off);

  } else {
    // Approaching but not yet in zone
    if (prox.distance < prox.zone_inner) {
      hud_text_draw_string(lx, ty, "TOO CLOSE", SMALL_SCALE, COL_YELLOW);
    } else {
      hud_text_draw_string(lx, ty, "APPROACH", SMALL_SCALE, COL_DIM);
    }
  }
}

// ============================================================================
// Public API
// ============================================================================

void hud_initialize(void) {
  hud_target_ = (entity_id_t)INVALID_ENTITY;
}

void hud_set_entity(entity_id_t entity_id) {
  hud_target_ = entity_id;
}

void hud_draw(void) {
  if (!is_valid_id(hud_target_))
    return;
  if (IS_PART(hud_target_))
    return;

  struct objects_data* od = entity_manager_get_objects();
  uint32_t idx = GET_ORDINAL(hud_target_);
  if (idx >= od->active)
    return;

  // Read entity data
  float vx = od->velocity_x[idx];
  float vy = od->velocity_y[idx];
  float speed = sqrtf(vx * vx + vy * vy);
  float health = (float)od->health[idx];

  // Layout: three panels aligned at the bottom of the screen.
  // The AP panel is taller, so it pokes up above the gauge panels.
  float total_w = HUD_GAUGE_PANEL_W + HUD_PANEL_GAP
                + HUD_AP_PANEL_W + HUD_PANEL_GAP
                + HUD_GAUGE_PANEL_W;
  float start_x = ((float)WINDOW_WIDTH - total_w) * 0.5f;
  float bottom_y = (float)WINDOW_HEIGHT - HUD_PANEL_MARGIN_Y;

  float gauge_y = bottom_y - HUD_GAUGE_PANEL_H;
  float ap_y    = bottom_y - HUD_AP_PANEL_H;

  float speed_x  = start_x;
  float ap_x     = speed_x + HUD_GAUGE_PANEL_W + HUD_PANEL_GAP;
  float health_x = ap_x + HUD_AP_PANEL_W + HUD_PANEL_GAP;

  _draw_gauge(speed_x, gauge_y, HUD_GAUGE_PANEL_W, speed, SPEED_MAX, "SPEED", _color_speed);
  _draw_nav_panel(ap_x, ap_y, HUD_AP_PANEL_H, HUD_AP_PANEL_W, hud_target_);
  _draw_gauge(health_x, gauge_y, HUD_GAUGE_PANEL_W, health, HEALTH_MAX, "HEALTH", _color_health);
}

void hud_remap_entity(uint32_t* remap, uint32_t old_active) {
  if (is_valid_id(hud_target_)) {
    uint32_t old_ord = GET_ORDINAL(hud_target_);
    if (old_ord < old_active) {
      uint32_t new_ord = remap[old_ord];
      if (new_ord != UINT32_MAX) {
        uint8_t type = GET_TYPE(hud_target_);
        hud_target_ = OBJECT_ID_WITH_TYPE(new_ord, type);
      } else {
        hud_target_ = (entity_id_t)INVALID_ENTITY;
      }
    }
  }
}

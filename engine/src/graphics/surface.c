#include "surface.h"
#include "entity/entity.h"
#include "platform/platform.h"
#include "debug/profiler.h"

#include <math.h>

#define INV_10000 (1.0f / 10000.0f)
#define HORIZON_FADE 0.15f

void surface_tick(void) {
  struct objects_data* od = entity_manager_get_objects();

  for (uint32_t i = 0; i < od->active; i++) {
    if (od->surface_idx[i] == 0xFFFF) continue;
    od->surface_rotation[i] += od->rotation_speed[i];
  }
}

// Project a unit sphere point to screen, returns depth (>0 = visible)
static float _project(float px, float py, float pz, float cr, float sr,
                      float cx, float cy, float radius,
                      float* out_x, float* out_y) {
  float depth = px * cr - py * sr;
  float horiz = px * sr + py * cr;
  *out_x = cx + radius * horiz;
  *out_y = cy + radius * pz;
  return depth;
}

static uint8_t _depth_alpha(float depth, uint8_t base_alpha) {
  if (depth > HORIZON_FADE) return base_alpha;
  return (uint8_t)(base_alpha * (depth / HORIZON_FADE));
}

void surface_draw_all(void) {
  PROFILE_ZONE("surface_draw_all");
  struct objects_data* od = entity_manager_get_objects();

  for (uint32_t i = 0; i < od->active; i++) {
    uint16_t sidx = od->surface_idx[i];
    if (sidx == 0xFFFF) continue;

    const struct surface_data* surf = _generated_get_surface(sidx);
    if (!surf) continue;

    float cx = od->position_orientation.position_x[i];
    float cy = od->position_orientation.position_y[i];
    float radius = od->position_orientation.radius[i];
    float rot = od->surface_rotation[i];

    float cr = cosf(rot);
    float sr = sinf(rot);

    for (uint16_t p = 0; p < surf->poly_count; p++) {
      const struct surface_polyline* poly = &surf->polys[p];
      if (poly->count < 2) continue;

      // Project first point
      float prev_px = poly->verts[0] * INV_10000;
      float prev_py = poly->verts[1] * INV_10000;
      float prev_pz = poly->verts[2] * INV_10000;
      float prev_sx, prev_sy;
      float prev_depth = _project(prev_px, prev_py, prev_pz, cr, sr, cx, cy, radius, &prev_sx, &prev_sy);

      for (uint16_t v = 1; v < poly->count; v++) {
        float cur_px = poly->verts[v * 3 + 0] * INV_10000;
        float cur_py = poly->verts[v * 3 + 1] * INV_10000;
        float cur_pz = poly->verts[v * 3 + 2] * INV_10000;
        float cur_sx, cur_sy;
        float cur_depth = _project(cur_px, cur_py, cur_pz, cr, sr, cx, cy, radius, &cur_sx, &cur_sy);

        if (prev_depth > 0.0f && cur_depth > 0.0f) {
          // Both visible: draw segment
          uint8_t alpha = _depth_alpha(prev_depth < cur_depth ? prev_depth : cur_depth, surf->color.a);
          color_t c = { surf->color.r, surf->color.g, surf->color.b, alpha };
          platform_renderer_draw_line(prev_sx, prev_sy, cur_sx, cur_sy, c);

        } else if (prev_depth > 0.0f && cur_depth <= 0.0f) {
          // Crossing from visible to hidden: interpolate crossing point
          float t = prev_depth / (prev_depth - cur_depth);
          float hx = prev_sx + t * (cur_sx - prev_sx);
          float hy = prev_sy + t * (cur_sy - prev_sy);
          color_t c = { surf->color.r, surf->color.g, surf->color.b, 40 };
          platform_renderer_draw_line(prev_sx, prev_sy, hx, hy, c);

        } else if (prev_depth <= 0.0f && cur_depth > 0.0f) {
          // Crossing from hidden to visible: interpolate crossing point
          float t = prev_depth / (prev_depth - cur_depth);
          float hx = prev_sx + t * (cur_sx - prev_sx);
          float hy = prev_sy + t * (cur_sy - prev_sy);
          color_t c = { surf->color.r, surf->color.g, surf->color.b, 40 };
          platform_renderer_draw_line(hx, hy, cur_sx, cur_sy, c);
        }
        // else: both hidden, skip

        prev_sx = cur_sx;
        prev_sy = cur_sy;
        prev_depth = cur_depth;
      }
    }
  }
  PROFILE_ZONE_END();
}

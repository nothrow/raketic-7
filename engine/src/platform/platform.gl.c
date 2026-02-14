#include "platform.h"
#include "debug/profiler.h"

#include <Windows.h>
#include <gl/GL.h>

#pragma comment(lib, "opengl32.lib")

#include "../generated/renderer.gen.h"
#include "../entity/entity.h"

static color_t white_ = { 255, 255, 255, 255 };

void _gl_initialize(void) {
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0, -1.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void platform_renderer_draw_models(size_t model_count, const color_t* colors,
                                   const position_orientation_t* position_orientation, const uint16_t* model_indices) {
  for (size_t i = 0; i < model_count; i++) {
    if (model_indices[i] == 0xFFFF) {
      continue;
    }

    glPushMatrix();

    // clang-format off
    float m[16] = {
      position_orientation->orientation_x[i], position_orientation->orientation_y[i], 0.0, 0.0,
      -position_orientation->orientation_y[i], position_orientation->orientation_x[i], 0.0, 0.0,
      0.0              , 0.0               , 1.0, 0.0,
      position_orientation->position_x[i]   , position_orientation->position_y[i]    , 0.0, 1.0,
    };
    // clang-format on

    glMultMatrixf(m);

    _generated_draw_model(colors ? colors[i] : white_, model_indices[i]);
    PROFILE_DRAW_CALL();

    glPopMatrix();
  }
}

void platform_debug_draw_line(float x1, float y1, float x2, float y2, color_t color) {
  glColor4ub(color.r, color.g, color.b, color.a);
  glLineWidth(1.0f);
  glBegin(GL_LINES);
  glVertex2f(x1, y1);
  glVertex2f(x2, y2);
  glEnd();
}

void platform_renderer_draw_stars(size_t count, const float* vertices, const color_t* colors) {
  if (count == 0) return;

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPointSize(1.0f);

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);

  glVertexPointer(2, GL_FLOAT, 0, vertices);
  glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);

  glDrawArrays(GL_POINTS, 0, (GLsizei)count);
  PROFILE_DRAW_CALL();

  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisable(GL_BLEND);
}

void platform_renderer_draw_beams(size_t count, const float* start_x, const float* start_y,
                                  const float* end_x, const float* end_y,
                                  const uint16_t* lifetime_ticks, const uint16_t* lifetime_max) {
  if (count == 0) return;

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // additive blending for glow effect
  glLineWidth(2.0f);

  glBegin(GL_LINES);
  for (size_t i = 0; i < count; i++) {
    // calculate alpha based on remaining lifetime
    float t = (float)lifetime_ticks[i] / (float)(lifetime_max[i] > 0 ? lifetime_max[i] : 1);
    uint8_t alpha = (uint8_t)(t * 255.0f);
    
    // cyan/blue laser color
    glColor4ub(100, 200, 255, alpha);
    glVertex2f(start_x[i], start_y[i]);
    glVertex2f(end_x[i], end_y[i]);
  }
  glEnd();
  PROFILE_DRAW_CALL();

  glLineWidth(1.0f);
  glDisable(GL_BLEND);
}

void platform_renderer_draw_line(float x1, float y1, float x2, float y2, color_t color) {
  glColor4ub(color.r, color.g, color.b, color.a);
  glBegin(GL_LINES);
  glVertex2f(x1, y1);
  glVertex2f(x2, y2);
  glEnd();
}

void platform_renderer_push_zoom(float zoom) {
  glPushMatrix();
  float inv = 1.0f / zoom;
  glTranslatef(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f, 0.0f);
  glScalef(inv, inv, 1.0f);
  glTranslatef(-WINDOW_WIDTH / 2.0f, -WINDOW_HEIGHT / 2.0f, 0.0f);
}

void platform_renderer_pop_zoom(void) {
  glPopMatrix();
}

void platform_renderer_report_stats(void) {
  PROFILE_DRAW_CALLS_RESET();
}

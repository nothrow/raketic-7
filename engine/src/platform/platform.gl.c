#include "platform.h"

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

void platform_renderer_draw_stars(size_t count, const float* x, const float* y, const uint8_t* alpha) {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPointSize(1.0f);
  
  glBegin(GL_POINTS);
  for (size_t i = 0; i < count; i++) {
    // Only draw if on screen (simple culling)
    if (x[i] >= 0 && x[i] < WINDOW_WIDTH && y[i] >= 0 && y[i] < WINDOW_HEIGHT) {
      glColor4ub(255, 255, 255, alpha[i]);
      glVertex2f(x[i], y[i]);
    }
  }
  glEnd();
  
  glDisable(GL_BLEND);
}
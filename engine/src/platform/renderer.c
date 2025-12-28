#include "platform.h"

#include <Windows.h>
#include <gl/GL.h>

#include "../generated/renderer.gen.h"

void platform_renderer_draw_models(size_t model_count, const color_t* colors, const vec2_t* positions,
                                   const vec2_t* orientations, const uint16_t* model_indices) {
  for (size_t i = 0; i < model_count; i++) {
    if (model_indices[i] != 0)
      continue;

    glColor4ubv((GLubyte*)(colors + i));

    glPushMatrix();

    // clang-format off
    float m[16] = {
      orientations[i].x, -orientations[i].y, 0.0, 0.0,
      orientations[i].y, orientations[i].x , 0.0, 0.0,
      0.0              , 0.0               , 1.0, 0.0,
      positions[i].x   , positions[i].y    , 0.0, 1.0,
    };
    // clang-format on

    glMultMatrixf(m);
    
    _generated_draw_model(colors[i], model_indices[i]);
    /*
    glBegin(GL_LINE_STRIP);
    glVertex2d(-5.0, -5.0);
    glVertex2d(5.0, -5.0);
    glVertex2d(0, 10.0);
    glVertex2d(-5.0, -5.0);
    glEnd();*/

    glPopMatrix();
  }
}

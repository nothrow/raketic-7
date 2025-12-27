#include "platform.h"

#include <Windows.h>
#include <gl/GL.h>


static const GLubyte color_white[] = { 255, 255, 255 };

void platform_renderer_draw_models(
  size_t model_count,
  const color_t* colors,
  const vec2_t* positions,
  const vec2_t* orientations,
  const uint16_t* model_indices
) {
  for (size_t i = 0; i < model_count; i++)
  {
    if (model_indices[i] != 0)
      continue;

    glColor4ubv((GLubyte*)(colors + i));

    glPushMatrix();
    double m[16] = {
      orientations[i].x, -orientations[i].y, 0.0, 0.0,
      orientations[i].y, orientations[i].x, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      positions[i].x, positions[i].y, 0.0, 1.0
    };

    glMultMatrixd(m);

    glBegin(GL_LINE_STRIP);
    glVertex2d(-5.0, -5.0);
    glVertex2d(5.0, -5.0);
    glVertex2d(0, 10.0);
    glVertex2d(-5.0, -5.0);
    glEnd();
    glPopMatrix();
  }
}

#include "platform.h"

#include <Windows.h>
#include <gl/GL.h>

void _gl_initialize(void) {
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0, -1.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

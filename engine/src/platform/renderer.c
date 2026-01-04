#include "platform.h"

#include <Windows.h>
#include <gl/GL.h>

#include "../generated/renderer.gen.h"
#include "../entity/entity.h"

static color_t white_ = { 255, 255, 255, 255 };

void platform_renderer_draw_models(size_t model_count, const color_t* colors,
                                   const position_orientation_t* position_orientation, const uint16_t* model_indices) {
  for (size_t i = 0; i < model_count; i++) {
    if (model_indices[i] == 0xFFFF) {
      continue;
    }

    glPushMatrix();

    // clang-format off
    float m[16] = {
      position_orientation->orientation_x[i], -position_orientation->orientation_y[i], 0.0, 0.0,
      position_orientation->orientation_y[i], position_orientation->orientation_x[i], 0.0, 0.0,
      0.0              , 0.0               , 1.0, 0.0,
      position_orientation->position_x[i]   , position_orientation->position_y[i]    , 0.0, 1.0,
    };
    // clang-format on

    glMultMatrixf(m);

    _generated_draw_model(colors ? colors[i] : white_, model_indices[i]);

    glPopMatrix();
  }
}

#include "graphics.h"
#include "entity/entity.h"
#include "platform/platform.h"

void graphics_engine_draw(void) {
  struct particles_data* pd = entity_manager_get_particles();

  platform_renderer_draw_models(
    pd->active,
    pd->position,
    pd->orientation,
    pd->model_idx
  );
}

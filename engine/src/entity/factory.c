#include "entity_internal.h"
#include "../generated/slots.gen.h"
#include "../generated/renderer.gen.h" // todo split

static void _clear_position_orientation(uint32_t idx) {
  struct objects_data* od = entity_manager_get_objects();
  od->position_orientation.position_x[idx] = 400.0f;
  od->position_orientation.position_y[idx] = 300.0f;
  od->position_orientation.orientation_x[idx] = 1.0f;
  od->position_orientation.orientation_y[idx] = 0.0f;
}

entity_id_t _generate_entity_by_model(entity_type_t type, uint16_t model) {
  struct objects_data* od = entity_manager_get_objects();
  struct parts_data* pd = entity_manager_get_parts();

  _ASSERT(od->active < od->capacity);

  uint32_t new_idx = od->active;
  od->type[new_idx] = type;
  od->model_idx[new_idx] = model;

  _clear_position_orientation(new_idx);

  entity_id_t ret = OBJECT_ID_WITH_TYPE(new_idx, type._);

  _generated_fill_slots(model, ret);

  // fill the models?
  for (size_t i = 0; i < od->parts_count[new_idx]; ++i) {
    uint32_t part_idx = od->parts_start_idx[new_idx] + i;
    if (pd->type[part_idx]._ == ENTITY_TYPE_PART_ENGINE) {
      pd->model_idx[part_idx] = MODEL_ENGINE_IDX;
    }
  }

  od->active += 1;

  return ret;
}

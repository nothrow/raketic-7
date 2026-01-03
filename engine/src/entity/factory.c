#include "entity_internal.h"

static void _clear_position_orientation(uint32_t idx)
{
  struct objects_data* od = entity_manager_get_objects();
  od->position_orientation.position_x[idx] = 400.0f;
  od->position_orientation.position_y[idx] = 300.0f;
  od->position_orientation.orientation_x[idx] = 1.0f;
  od->position_orientation.orientation_y[idx] = 0.0f;
}

entity_id_t _generate_entity_by_model(entity_type_t type, uint16_t model) {
  struct objects_data* od = entity_manager_get_objects();
  _ASSERT(od->active < od->capacity);

  uint32_t new_idx = od->active;
  od->type[new_idx] = type;
  od->model_idx[new_idx] = model;

  _clear_position_orientation(new_idx);

  entity_id_t ret = ID_WITH_TYPE(new_idx, type._);

  od->active += 1;

  return ret;
}

#include "entity.h"


#define MAXSIZE 65535
#define NONEXISTENT ((size_t)(-1))

typedef struct {

  struct objects_data objects;

} entity_manager_t;

static entity_manager_t manager_ = { 0 };

static void objects_data_initialize(struct objects_data* data)
{
  data->position = retrieve_memory(sizeof(vec2_t) * MAXSIZE);
  data->velocity = retrieve_memory(sizeof(vec2_t) * MAXSIZE);
  data->acceleration = retrieve_memory(sizeof(vec2_t) * MAXSIZE);
  data->model_idx = retrieve_memory(sizeof(int) * MAXSIZE);
  data->orientation = retrieve_memory(sizeof(vec2_t) * MAXSIZE);
  data->mass = retrieve_memory(sizeof(double) * MAXSIZE);
  data->radius = retrieve_memory(sizeof(double) * MAXSIZE);

  data->active = 0;
  data->capacity = MAXSIZE;
}

void entity_manager_initialize(void)
{
  objects_data_initialize(&manager_.objects);
}

#include "entity.h"


#define MAXSIZE 65535
#define NONEXISTENT ((size_t)(-1))

typedef struct {

  struct objects_data objects;
  struct particles_data particles;

} entity_manager_t;

static entity_manager_t manager_ = { 0 };

static void objects_data_initialize(struct objects_data* data)
{
  data->position = retrieve_memory(sizeof(vec2_t) * MAXSIZE);
  data->velocity = retrieve_memory(sizeof(vec2_t) * MAXSIZE);
  data->thrust = retrieve_memory(sizeof(double) * MAXSIZE);
  data->model_idx = retrieve_memory(sizeof(uint16_t) * MAXSIZE);
  data->orientation = retrieve_memory(sizeof(vec2_t) * MAXSIZE);
  data->mass = retrieve_memory(sizeof(double) * MAXSIZE);
  data->radius = retrieve_memory(sizeof(double) * MAXSIZE);

  data->active = 0;
  data->capacity = MAXSIZE;
}

static void particles_data_initialize(struct particles_data* data)
{
  data->position = retrieve_memory(sizeof(vec2_t) * MAXSIZE);
  data->velocity = retrieve_memory(sizeof(vec2_t) * MAXSIZE);
  data->lifetime_ticks = retrieve_memory(sizeof(uint16_t) * MAXSIZE);
  data->model_idx = retrieve_memory(sizeof(uint16_t) * MAXSIZE);
  data->orientation = retrieve_memory(sizeof(vec2_t) * MAXSIZE);
  data->active = 0;
  data->capacity = MAXSIZE;
}

void entity_manager_initialize(void)
{
  objects_data_initialize(&manager_.objects);
  particles_data_initialize(&manager_.particles);
}


struct particles_data* entity_manager_get_particles(void)
{
  return &manager_.particles;
}

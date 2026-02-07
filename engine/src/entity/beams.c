#include "platform/platform.h"
#include "beams.h"

static struct beams_data beams_ = { 0 };

static void _beams_data_initialize(void) {
  beams_.start_x = platform_retrieve_memory(sizeof(float) * MAX_BEAMS);
  beams_.start_y = platform_retrieve_memory(sizeof(float) * MAX_BEAMS);
  beams_.end_x = platform_retrieve_memory(sizeof(float) * MAX_BEAMS);
  beams_.end_y = platform_retrieve_memory(sizeof(float) * MAX_BEAMS);
  beams_.lifetime_ticks = platform_retrieve_memory(sizeof(uint16_t) * MAX_BEAMS);
  beams_.lifetime_max = platform_retrieve_memory(sizeof(uint16_t) * MAX_BEAMS);
  beams_.active = 0;
  beams_.capacity = MAX_BEAMS;
}

void beams_create(float start_x, float start_y, float end_x, float end_y, uint16_t lifetime) {
  if (beams_.active >= beams_.capacity) {
    return; // drop if full
  }
  
  uint32_t idx = beams_.active++;
  beams_.start_x[idx] = start_x;
  beams_.start_y[idx] = start_y;
  beams_.end_x[idx] = end_x;
  beams_.end_y[idx] = end_y;
  beams_.lifetime_ticks[idx] = lifetime;
  beams_.lifetime_max[idx] = lifetime;
}

struct beams_data* beams_get_data(void) {
  return &beams_;
}

static void _beams_tick(void) {
  // decrement lifetime and compact
  uint32_t write_idx = 0;
  
  for (uint32_t i = 0; i < beams_.active; i++) {
    if (beams_.lifetime_ticks[i] > 1) {
      // copy to write position (compacting)
      if (write_idx != i) {
        beams_.start_x[write_idx] = beams_.start_x[i];
        beams_.start_y[write_idx] = beams_.start_y[i];
        beams_.end_x[write_idx] = beams_.end_x[i];
        beams_.end_y[write_idx] = beams_.end_y[i];
        beams_.lifetime_max[write_idx] = beams_.lifetime_max[i];
      }
      beams_.lifetime_ticks[write_idx] = beams_.lifetime_ticks[i] - 1;
      write_idx++;
    }
  }
  
  beams_.active = write_idx;
}

static void _beams_dispatch(entity_id_t id, message_t msg) {
  (void)id;
  switch (msg.message) {
    case MESSAGE_BROADCAST_120HZ_AFTER_PHYSICS:
      _beams_tick();
      break;
  }
}

void beams_entity_initialize(void) {
  _beams_data_initialize();
  entity_manager_vtables[ENTITY_TYPE_BEAMS].dispatch_message = _beams_dispatch;
}

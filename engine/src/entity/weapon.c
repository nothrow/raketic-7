#include "platform/platform.h"
#include "platform/math.h"
#include "weapon.h"
#include "beams.h"
#include "rocket.h"
#include "particles.h"
#include "../generated/renderer.gen.h"

static void _weapon_fire_laser(uint32_t part_idx, struct weapon_data* wd, struct parts_data* pd, struct objects_data* od) {
  (wd);

  uint32_t parent_idx = GET_ORDINAL(pd->parent_id[part_idx]);
  
  // get weapon world position
  float wx = pd->world_position_orientation.position_x[part_idx];
  float wy = pd->world_position_orientation.position_y[part_idx];
  
  // get parent orientation (firing direction)
  float ox = od->position_orientation.orientation_x[parent_idx];
  float oy = od->position_orientation.orientation_y[parent_idx];
  
  // calculate beam end point
  float beam_length = 500.0f;
  float end_x = wx + ox * beam_length;
  float end_y = wy + oy * beam_length;
  
  // create the beam (lasts for a short time)
  uint16_t beam_lifetime = 12; // ~0.1 seconds at 120Hz
  beams_create(wx, wy, end_x, end_y, beam_lifetime, parent_idx);
}

static void _weapon_fire_rocket(uint32_t part_idx, struct weapon_data* wd, struct parts_data* pd, struct objects_data* od) {
  uint32_t parent_idx = GET_ORDINAL(pd->parent_id[part_idx]);
  
  // get weapon world position
  float wx = pd->world_position_orientation.position_x[part_idx];
  float wy = pd->world_position_orientation.position_y[part_idx];
  
  // get parent orientation and velocity
  float ox = od->position_orientation.orientation_x[parent_idx];
  float oy = od->position_orientation.orientation_y[parent_idx];
  float parent_vx = od->velocity_x[parent_idx];
  float parent_vy = od->velocity_y[parent_idx];
  
  // forward offset to clear parent ship's collision radius
  float spawn_offset = 20.0f;
  wx += ox * spawn_offset;
  wy += oy * spawn_offset;
  
  // calculate rocket velocity (parent velocity + projectile speed in firing direction)
  float vx = parent_vx + ox * wd->projectile_speed;
  float vy = parent_vy + oy * wd->projectile_speed;
  
  // spawn the rocket
  rocket_create(wx, wy, vx, vy, ox, oy, wd->projectile_model, wd->smoke_model,
                wd->rocket_thrust, wd->rocket_fuel_ticks, wd->rocket_lifetime_ticks);
}

static void _weapon_set_firing(entity_id_t id, bool firing) {
  struct parts_data* pd = entity_manager_get_parts();
  
  if (IS_PART(id)) {
    // single part
    uint32_t idx = GET_ORDINAL(id);
    struct weapon_data* wd = (struct weapon_data*)(pd->data[idx].data);
    wd->firing = firing ? 1 : 0;
  } else {
    // all weapon parts of an entity
    uint32_t object_idx = GET_ORDINAL(id);
    struct objects_data* od = entity_manager_get_objects();
    
    for (uint32_t i = od->parts_start_idx[object_idx];
         i < od->parts_start_idx[object_idx] + od->parts_count[object_idx]; ++i) {
      if (pd->type[i]._ == ENTITY_TYPE_PART_WEAPON) {
        struct weapon_data* wd = (struct weapon_data*)(pd->data[i].data);
        wd->firing = firing ? 1 : 0;
      }
    }
  }
}

static void _weapon_tick(void) {
  struct parts_data* pd = entity_manager_get_parts();
  struct objects_data* od = entity_manager_get_objects();
  
  for (size_t i = 0; i < pd->active; i++) {
    if (pd->type[i]._ == ENTITY_TYPE_PART_WEAPON) {
      struct weapon_data* wd = (struct weapon_data*)(pd->data[i].data);
      
      // decrement cooldown
      if (wd->cooldown_remaining > 0) {
        wd->cooldown_remaining--;
      }
      
      // if firing and cooldown is ready, fire
      if (wd->firing && wd->cooldown_remaining == 0) {
        wd->cooldown_remaining = wd->cooldown_ticks;
        
        switch (wd->weapon_type) {
          case WEAPON_TYPE_LASER:
            _weapon_fire_laser((uint32_t)i, wd, pd, od);
            break;
          case WEAPON_TYPE_ROCKET:
            _weapon_fire_rocket((uint32_t)i, wd, pd, od);
            break;
        }
      }
    }
  }
}

static void _weapon_part_dispatch(entity_id_t id, message_t msg) {
  switch (msg.message) {
    case MESSAGE_WEAPON_FIRE:
      _weapon_set_firing(id, msg.data_a > 0);
      break;
      
    case MESSAGE_BROADCAST_120HZ_BEFORE_PHYSICS:
      _weapon_tick();
      break;
  }
}

void weapon_part_entity_initialize(void) {
  entity_manager_vtables[ENTITY_TYPE_PART_WEAPON].dispatch_message = _weapon_part_dispatch;
}

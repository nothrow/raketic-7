#pragma once

#include "entity/entity.h"

void hud_initialize(void);
void hud_set_entity(entity_id_t entity_id);
void hud_draw(void);
void hud_remap_entity(uint32_t* remap, uint32_t old_active);

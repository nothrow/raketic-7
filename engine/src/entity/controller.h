#pragma once

#include "entity_internal.h"

void controller_entity_initialize(void);
void controller_set_entity(entity_id_t entity_id);
void controller_remap_entity(uint32_t* remap, uint32_t old_active);

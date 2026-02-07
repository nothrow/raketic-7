#pragma once

#include <stdbool.h>
#include "core/core.h"

// with type
typedef struct {
  uint32_t _;
} entity_id_t;

typedef struct {
  uint8_t _;
} entity_type_t;

static inline uint8_t GET_TYPE(entity_id_t tid) {
  return (uint8_t)(((tid._) >> 24) & 0xFF);
}

static inline bool IS_PART(entity_id_t tid) {
  return (((tid._) >> 23) & 1);
}

static inline uint32_t GET_ORDINAL(entity_id_t tid) {
  return (uint32_t)((tid._) & 0x007FFFFF);
}

static inline entity_id_t TYPE_BROADCAST(entity_type_t type) {
  _ASSERT((type._) <= 0xFF);
  entity_id_t ret = { ._ = (((type._) & 0xFF) << 24) | 0x007FFFFF };
  return ret;
}

static inline entity_id_t OBJECT_ID_WITH_TYPE(uint32_t id, uint8_t type) {
  _ASSERT((id) <= 0x007FFFFF);
  _ASSERT((type) <= 0xFF);

  entity_id_t ret = { ._ = ((((type) & 0xFF) << 24) | ((id) & 0x007FFFFF)) };
  return ret;
}

static inline entity_id_t PART_ID_WITH_TYPE(uint32_t id, uint8_t type) {
  _ASSERT((id) <= 0x007FFFFF);
  _ASSERT((type) <= 0xFF);

  entity_id_t ret = { ._ = ((((type) & 0xFF) << 24) | 0x800000 | ((id) & 0x007FFFFF)) };

  return ret;
}

// makes the target all parts of given type of given entity
static inline entity_id_t PARTS_OF_TYPE(entity_id_t entity, entity_type_t part) {
  _ASSERT(!IS_PART(entity));

  return OBJECT_ID_WITH_TYPE(GET_ORDINAL(entity), part._);
}

static inline bool is_valid_id(entity_id_t id) {
  return id._ != 0xFFFFFFFF;
}

#define INVALID_ENTITY { 0xFFFFFFFF }

enum entity_type {
  ENTITY_TYPE_ANY = 0,
  ENTITY_TYPE_CONTROLLER,
  ENTITY_TYPE_CAMERA,
  ENTITY_TYPE_PARTICLES,
  ENTITY_TYPE_SHIP,
  ENTITY_TYPE_PART_ENGINE,
  ENTITY_TYPE_PART_WEAPON,
  ENTITY_TYPE_PLANET,
  ENTITY_TYPE_ROCKET,
  ENTITY_TYPE_BEAMS,
  ENTITY_TYPE_ASTEROID,
  ENTITY_TYPE_PART_CHUNK,
  ENTITY_TYPE_COUNT,
};

static entity_type_t ENTITY_TYPEREF_SHIP = { ENTITY_TYPE_SHIP };
static entity_type_t ENTITY_TYPEREF_PARTICLES = { ENTITY_TYPE_PARTICLES };
static entity_type_t PART_TYPEREF_ENGINE = { ENTITY_TYPE_PART_ENGINE };
static entity_type_t PART_TYPEREF_WEAPON = { ENTITY_TYPE_PART_WEAPON };
static entity_type_t ENTITY_TYPEREF_PLANET = { ENTITY_TYPE_PLANET };
static entity_type_t ENTITY_TYPEREF_ROCKET = { ENTITY_TYPE_ROCKET };
static entity_type_t ENTITY_TYPEREF_BEAMS = { ENTITY_TYPE_BEAMS };
static entity_type_t ENTITY_TYPEREF_ASTEROID = { ENTITY_TYPE_ASTEROID };
static entity_type_t PART_TYPEREF_CHUNK = { ENTITY_TYPE_PART_CHUNK };

#define WEAPON_TYPE_LASER  0
#define WEAPON_TYPE_ROCKET 1



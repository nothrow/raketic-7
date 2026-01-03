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

static inline uint32_t GET_ORDINAL(entity_id_t tid) {
  return (uint32_t)((tid._) & 0x00FFFFFF);
}

static inline entity_id_t ID_WITH_TYPE(uint32_t id, uint8_t type) {
  _ASSERT((id) <= 0x00FFFFFF);
  _ASSERT((type) <= 0xFF);

  entity_id_t ret = { ._ = ((((type) & 0xFF) << 24) | ((id) & 0x00FFFFFF)) };
  return ret;
}

static inline bool is_valid_id(entity_id_t id) {
  return id._ != 0xFFFFFFFF;
}

#define INVALID_ENTITY                                                                                                 \
  { 0xFFFFFFFF }

enum entity_type {
  ENTITY_TYPE_ANY = 0,
  ENTITY_TYPE_SHIP,
  ENTITY_TYPE_CONTROLLER,
  ENTITY_TYPE_COUNT,
};

static entity_type_t ENTITY_TYPEREF_SHIP = { ENTITY_TYPE_SHIP };

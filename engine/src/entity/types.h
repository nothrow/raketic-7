#pragma once

#include <stdbool.h>

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

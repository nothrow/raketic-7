#include "platform.h"
#include "debug/profiler.h"

#include <Windows.h>

static void* fixed_heap_ = NULL;
static size_t allocated_size_ = 0;


#define MEMORY_MAX_SIZE (30 * 1024 * 1024)

void _memory_initialize(void) {
  fixed_heap_ = VirtualAlloc(NULL, MEMORY_MAX_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  _ASSERT(fixed_heap_ != NULL);
}

void* platform_retrieve_memory(size_t memory_size) {
  size_t aligned_offset = (allocated_size_ + 15) & ~(size_t)15;

  _ASSERT(aligned_offset + memory_size <= MEMORY_MAX_SIZE);

  void* ptr = (char*)fixed_heap_ + aligned_offset;
  allocated_size_ = aligned_offset + memory_size;

  PROFILE_ALLOC(ptr, memory_size);
  return ptr;
}

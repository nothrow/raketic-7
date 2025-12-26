#include "core.h"
#include "platform/platform.h"

#include <stdlib.h>
#include <Windows.h>

#define MAXSIZE 10*1024*1024

static void* fixed_heap = NULL;
static size_t allocated_size = 0;

void memory_initialize(void) {
  fixed_heap = VirtualAlloc(NULL, MAXSIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  assert(fixed_heap != NULL);
}

void* retrieve_memory(size_t memory_size) {
  assert(allocated_size + memory_size < MAXSIZE);

  void* ptr = (char*)fixed_heap + allocated_size;
  allocated_size += memory_size;
  return ptr;
}

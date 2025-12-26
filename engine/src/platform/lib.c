#include <windows.h>

#include "platform.h"

void clear_memory(void* ptr, size_t size) {
  RtlZeroMemory(ptr, size);
}

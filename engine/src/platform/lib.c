#include "platform.h"

#include <Windows.h>

void platform_clear_memory(void* ptr, size_t size) {
  RtlZeroMemory(ptr, size);
}

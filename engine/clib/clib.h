#pragma once


#include <assert.h>
#include <stdbool.h>

#define _ASSERT assert

#ifdef NDEBUG
	#define _VERIFY(x, msg) (x)
#else
	#define _VERIFY(x, msg) assert((x) && (msg))
#endif

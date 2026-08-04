#pragma once




//
//dependencies

#include <string>
#include <math.h>
#include <memory.h>	//for memset etc

#include "gcc.h"
#include "intel.h"




//
//linux specific

#undef ALIGN

#if defined (_WIN32)
#undef REFLEX_INLINE
#define REFLEX_FORCEINLINE inline
#endif

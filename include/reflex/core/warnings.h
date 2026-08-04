#pragma once

#include "[require].h"




//
//warnings

namespace Reflex
{

	inline bool not_on_heap = true;

}




//
//macros

#define REFLEX_HEAPCHECK(OUTPUT, THIS, METHOD, OBJECT) REFLEX_DEBUG_WARN(OUTPUT, Reflex::not_on_heap, (OBJECT).GetAllocator(), THIS->object_t->tname, "::", REFLEX_STRINGIFY(METHOD), " @", (OBJECT).object_t->tname);

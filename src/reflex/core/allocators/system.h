#pragma once

#include "reflex/core/allocator/allocators.h"




REFLEX_NS(Reflex)

struct SystemAllocator : public Allocator
{
	virtual void * OnAlloc(UIntNative n, const AllocInfo & info) override { return REFLEX_ALLOC16(n); }

	virtual void OnFree(void * ptr) override { REFLEX_FREE16(RemoveConst(ptr)); }
};

REFLEX_END

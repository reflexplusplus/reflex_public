#pragma once

#include "../types.h"




//
//Secondary API

namespace Reflex
{

	void MemClear(void * address, UIntNative size);

	void MemCopy(const void * source, void * destination, UIntNative size);

	void MemMove(const void * source, void * destination, UIntNative size);

	bool MemCompare(const void * a, const void * b, UIntNative size);

}




//
//impl

REFLEX_INLINE void Reflex::MemClear(void * address, UIntNative size)
{
	memset(address, 0, size);
}

REFLEX_INLINE void Reflex::MemCopy(const void * source, void * destination, UIntNative size)
{
	memcpy(destination, source, size);
}

REFLEX_INLINE void Reflex::MemMove(const void * source, void * destination, UIntNative size)
{
	memmove(destination, source, size);
}

REFLEX_INLINE bool Reflex::MemCompare(const void * a, const void * b, UIntNative size)
{
	return memcmp(a, b, size) == 0;
}

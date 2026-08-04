#pragma once

#include "../object/object.h"




//
//Secondary API

namespace Reflex
{

	class Allocator;

}




//
//Allocator

class Reflex::Allocator : public Object
{
public:

	void * Allocate(UIntNative n, const AllocInfo & info);

	template <bool ASSUME_NOTNULL = false> void Free(void * ptr);



protected:

	virtual void * OnAlloc(UIntNative n, const AllocInfo & info) = 0;

	virtual void OnFree(void * ptr) = 0;

};




//
//impl

REFLEX_INLINE void * Reflex::Allocator::Allocate(UIntNative n, const AllocInfo & info)
{
	return OnAlloc(n, info);
}

template <bool ASSUME_NOTNULL> REFLEX_INLINE void Reflex::Allocator::Free(void * ptr)
{
	if constexpr (ASSUME_NOTNULL)
	{
		OnFree(ptr);
	}
	else if (ptr)
	{
		OnFree(ptr);
	}
}

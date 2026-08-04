#pragma once

#include "../view.h"
#include "../../detail/functions/allocate.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE> void Wipe(ArrayRegion <TYPE> region);

	template <class TYPE> void Fill(ArrayRegion <TYPE> region, const TYPE & value);

	template <class TYPE> void Copy(ArrayView <TYPE> src, ArrayRegion <TYPE> dst);

}




//
//impl

template <class TYPE> inline void Reflex::Wipe(ArrayRegion <TYPE> region)
{
	if constexpr (kIsRawConstructible<TYPE>)
	{
		MemClear(region.data, sizeof(TYPE) * region.size);
	}
	else
	{
		REFLEX_LOOP_PTR(region.data, itr, region.size)
		{
			Detail::Constructor<TYPE>::Reconstruct(*itr);
		}
	}
}

template <class TYPE> inline void Reflex::Copy(ArrayView <TYPE> src, ArrayRegion <TYPE> dst)
{
	REFLEX_ASSERT(src.size == dst.size);

	if constexpr (kIsRawCopyable<TYPE>)
	{
		MemCopy(src.data, dst.data, sizeof(TYPE) * dst.size);
	}
	else
	{
		auto pdst = dst.data;

		REFLEX_LOOP_PTR(src.data, psrc, dst.size) *pdst++ = *psrc;
	}
}

template <class TYPE> inline void Reflex::Fill(ArrayRegion <TYPE> region, const TYPE & value)
{
	REFLEX_LOOP_PTR(region.data, itr, region.size) *itr = value;
}

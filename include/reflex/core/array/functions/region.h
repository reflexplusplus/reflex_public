#pragma once

#include "../view.h"
#include "../../detail/functions/allocate.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE> void Wipe(const ArrayRegion <TYPE> & region);

	template <class TYPE> void Fill(const ArrayRegion <TYPE> & region, const TYPE & value);

	template <class TYPE> void Copy(const ArrayRegion <TYPE> & region, const TYPE * values);

}




//
//impl

template <class TYPE> inline void Reflex::Wipe(const ArrayRegion <TYPE> & region)
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

template <class TYPE> inline void Reflex::Copy(const ArrayRegion <TYPE> & region, const TYPE * values)
{
	if constexpr (kIsRawCopyable<TYPE>)
	{
		MemCopy(values, region.data, sizeof(TYPE) * region.size);
	}
	else
	{
		REFLEX_LOOP_PTR(region.data, pdst, region.size) *pdst = *values++;
	}
}

template <class TYPE> inline void Reflex::Fill(const ArrayRegion <TYPE> & region, const TYPE & value)
{
	REFLEX_LOOP_PTR(region.data, itr, region.size) *itr = value;
}

#pragma once

#include "../array.h"




//
//Primary API

namespace Reflex
{

	template <AllocatePolicy POLICY = kAllocateOver, class TYPE> ArrayRegion <TYPE> Extend(Array <TYPE> & data, UInt n);


	template <class ARRAY> void Reverse(ARRAY && data);


	template <class TYPE> void Reorder(Array <TYPE> & data, UInt from, UInt to);

	template <class TYPE> void Reorder(ArrayRegion <TYPE> data, UInt from, UInt to);


	template <class TYPE> void Swap(Array <TYPE> & a, Array <TYPE> & b);	//swap overload

}




//
//impl

REFLEX_NS(Reflex::Detail)

template <typename TYPE> inline void Reverse(ArrayRegion <TYPE> data)
{
	REFLEX_LOOP(idx, data.size / 2)
	{
		Swap(data[idx], data[data.size - idx - 1]);
	}
}

REFLEX_END

template <Reflex::AllocatePolicy POLICY, class TYPE> REFLEX_INLINE Reflex::ArrayRegion <TYPE> Reflex::Extend(Array <TYPE> & data, UInt n)
{
	UInt size = data.GetSize();

	data.template Expand<POLICY>(n);

	return { data.GetData() + size, n };
}

template <typename ARRAY> inline void Reflex::Reverse(ARRAY && data)
{
	Detail::Reverse(ToRegion(data));
}

template <class TYPE> inline void Reflex::Reorder(Array <TYPE> & data, UInt from, UInt to)
{
	TYPE temp = data[from];

	data.Remove(from);

	data.Insert(to, std::move(temp));
}

template <class TYPE> inline void Reflex::Reorder(ArrayRegion <TYPE> data, UInt from, UInt to)
{
    TYPE temp = data[from];

    if (from < to) 
	{
		for (UInt i = from; i < to; ++i) 
		{
			data[i] = data[i + 1];
		}
    }
    else if (from > to) 
	{
		for (UInt i = from; i > to; --i)
		{
			data[i] = data[i - 1];
		}
    }

    data[to] = temp;
}

template <class TYPE> REFLEX_INLINE void Reflex::Swap(Array <TYPE> & a, Array <TYPE> & b)
{
	a.Swap(b);
}

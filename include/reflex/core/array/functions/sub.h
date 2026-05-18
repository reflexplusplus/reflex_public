#pragma once

#include "convert.h"




//
//Primary API

namespace Reflex
{

	template <bool BOUNDS_CHECK = false, class ARRAY> /*ArrayView*/ auto Left(const ARRAY & array, UInt n);

	template <bool BOUNDS_CHECK = false, class ARRAY> /*ArrayView*/ auto Mid(const ARRAY & array, UInt pos);

	template <bool BOUNDS_CHECK = false, class ARRAY> /*ArrayView*/ auto Mid(const ARRAY & array, UInt pos, UInt n);

	template <bool BOUNDS_CHECK = false, class ARRAY> /*ArrayView*/ auto Right(const ARRAY & array, UInt n);


	template <bool BOUNDS_CHECK = false, class ARRAY> /*Pair <ArrayView>*/ auto Splice(const ARRAY & array, UInt position);

	template <bool BOUNDS_CHECK = false, class ARRAY> /*Pair <ArrayView>*/ auto ReverseSplice(const ARRAY & array, UInt position);


	template <class TYPE> ArrayRegion <TYPE> Inc(ArrayRegion <TYPE> & itr, UInt n);

	template <bool BOUNDS_CHECK = false, class TYPE> ArrayRegion <TYPE> Nudge(const ArrayRegion <TYPE> & itr, Int n = 1);

}




//
//impl

REFLEX_NS(Reflex::Detail)

template <bool BOUNDS_CHECK, class TYPE> inline ArrayRegion <TYPE> Left(const ArrayRegion <TYPE> & region, UInt n)
{
	if constexpr (BOUNDS_CHECK)
	{
		n = Min(n, region.size);
	}
	else
	{
		REFLEX_ASSERT(n <= region.size);
	}

	return { region.data, n };
}

template <bool BOUNDS_CHECK, class TYPE> inline ArrayRegion <TYPE> Mid(const ArrayRegion <TYPE> & region, UInt pos)
{
	if constexpr (BOUNDS_CHECK)
	{
		pos = Min(pos, region.size);
	}
	else
	{
		REFLEX_ASSERT(pos <= region.size);
	}

	return { region.data + pos, region.size - pos };
}

template <bool BOUNDS_CHECK, class TYPE> inline ArrayRegion <TYPE> Mid(const ArrayRegion <TYPE> & region, UInt pos, UInt n)
{
	if constexpr (BOUNDS_CHECK)
	{
		pos = Min(pos, region.size);

		n = Min(region.size - pos, n);
	}
	else
	{
		REFLEX_ASSERT((pos + n) <= region.size);
	}

	return { region.data + pos, n };
}

template <bool BOUNDS_CHECK, class TYPE> inline ArrayRegion <TYPE> Right(const ArrayRegion <TYPE> & region, UInt n)
{
	if constexpr (BOUNDS_CHECK)
	{
		n = Min(n, region.size);
	}
	else
	{
		REFLEX_ASSERT(n <= region.size);
	}

	return { region.data + (region.size - n), n };
}

template <bool BOUNDS_CHECK, class TYPE> REFLEX_INLINE Pair < ArrayRegion <TYPE> > Splice(const ArrayRegion <TYPE> & region, UInt position)
{
	Pair < ArrayRegion <TYPE> > pair = { region, region };

	if constexpr (BOUNDS_CHECK)
	{
		position = Min(position, region.size);
	}
	else
	{
		REFLEX_ASSERT(position <= region.size);
	}

	pair.a.size = position;

	pair.b.data += position;

	pair.b.size -= position;

	return pair;
}

REFLEX_END

template <bool BOUNDS_CHECK, class ARRAY> inline auto Reflex::Left(const ARRAY & array, UInt n)
{
	return Detail::Left<BOUNDS_CHECK>(ToView(array), n);
}

template <bool BOUNDS_CHECK, class ARRAY> inline auto Reflex::Mid(const ARRAY & array, UInt pos)
{
	return Detail::Mid<BOUNDS_CHECK>(ToView(array), pos);
}

template <bool BOUNDS_CHECK, class ARRAY> inline auto Reflex::Mid(const ARRAY & array, UInt pos, UInt n)
{
	return Detail::Mid<BOUNDS_CHECK>(ToView(array), pos, n);
}

template <bool BOUNDS_CHECK, class ARRAY> inline auto Reflex::Right(const ARRAY & array, UInt n)
{
	return Detail::Right<BOUNDS_CHECK>(ToView(array), n);
}

template <bool BOUNDS_CHECK, class ARRAY> inline auto Reflex::Splice(const ARRAY & array, UInt position)
{
	return Detail::Splice<BOUNDS_CHECK>(ToView(array), position);
}

template <bool BOUNDS_CHECK, class ARRAY> inline auto Reflex::ReverseSplice(const ARRAY & array, UInt position)
{
	auto view = ToView(array);

	return Detail::Splice<BOUNDS_CHECK>(view, view.size - position);
}

template <class TYPE> REFLEX_INLINE Reflex::ArrayRegion <TYPE> Reflex::Inc(ArrayRegion <TYPE> & itr, UInt n)
{
	REFLEX_ASSERT(itr.size >= n);

	auto t = itr;

	t.size = n;

	itr = Nudge(itr, n);

	return t;
}

template <bool BOUNDS_CHECK, class TYPE> REFLEX_INLINE Reflex::ArrayRegion <TYPE> Reflex::Nudge(const ArrayRegion <TYPE> & itr, Int n)
{
	if constexpr (BOUNDS_CHECK)
	{
		n = Min<Int>(itr.size, n);
	}

	return { itr.data + n, itr.size - n };
}

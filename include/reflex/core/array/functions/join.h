#pragma once

#include "search.h"




//
//Primary API

namespace Reflex
{

	template <class... VARGS> auto Join(const VARGS & ... vargs);

}




//
//impl

REFLEX_NS(Reflex::Detail)

struct Joiner
{
	template <class FIRST, class... VARGS> static auto ToView(const FIRST & first, const VARGS & ... vargs)
	{
		return Reflex::ToView(first);
	}

	template <class TYPE, class P> static void Count(ArrayView <TYPE> * & ptr, UInt & length, const P & p)
	{
		*ptr = Reflex::ToView(p);

		length += ptr->size;
	}

	template <class TYPE, class P, class... VARGS> static void Count(ArrayView <TYPE> * & ptr, UInt & length, const P & p, const VARGS & ... vargs)
	{
		*ptr = Reflex::ToView(p);

		length += ptr->size;

		Count(++ptr, length, vargs...);
	}

	template <class... VARGS> inline static auto Join(Allocator & allocator, const VARGS & ... vargs)
	{
		using Type = NonConstT <typename decltype(ToView(vargs...))::Type>;

		ArrayView <Type> refs[sizeof...(VARGS)];


		UInt length = 0;

		auto t = refs + 0;

		Count<Type>(t, length, vargs...);


		Array <Type> rtn(allocator);

		rtn.Allocate(length);

		REFLEX_LOOP(idx, sizeof...(VARGS))
		{
			rtn.Append(refs[idx]);
		}

		return rtn;
	}
};

REFLEX_END

template <class... VARGS> REFLEX_INLINE auto Reflex::Join(const VARGS & ... vargs)
{
	return Detail::Joiner::Join(g_default_allocator, vargs...);
}

template <class CASEPOLICY, class TYPE> inline Reflex::Array <TYPE> Reflex::Detail::ReplaceRegion(const ArrayView <TYPE> & value, const ArrayView <TYPE> & from, const ArrayView <TYPE> & to)
{
	Array <TYPE> rtn(value);

	UInt position = 0;

	while (auto idx = SearchRegion<CASEPOLICY>(Mid(rtn, position), from))
	{
		position += idx.value;

		rtn = Join(Left(rtn, position), to, Mid(rtn, position + from.size));

		position += to.size;
	}

	return rtn;
}

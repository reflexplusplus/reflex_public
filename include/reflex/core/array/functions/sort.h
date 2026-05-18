#pragma once

#include "../array.h"




//
//Primary API

namespace Reflex
{

	template <class COMPARE = StandardCompare, class ARRAY> void Sort(ARRAY & values);	//not as fast as std::sort but smaller 

	template <class ARRAY, class LT> void Sort(ARRAY & values, LT lt);

}




//
//impl

REFLEX_NS(Reflex::Detail)

template <class TYPE, class LT> inline void Sort(ArrayRegion <TYPE> values, LT lt)
{
	struct Impl
	{
		REFLEX_INLINE static void Swap(ArrayRegion <TYPE> & values, UInt a, UInt b)
		{
			auto t = values[a];

			values[a] = std::move(values[b]);

			values[b] = std::move(t);
		}

		REFLEX_INLINE static Int Partition(ArrayRegion <TYPE> & values, LT lt, Int low, Int high)
		{
			const auto pivot = values[high];

			Int i = low - 1;

			for (Int j = low; j <= high - 1; j++)
			{
				if (lt(values[j], pivot))
				{
					i++;

					Swap(values, i, j);
				}
			}

			Swap(values, i + 1, high);

			return i + 1;
		}

		static void Recurse(ArrayRegion <TYPE> & values, LT lt, Int low, Int high)
		{
			if (low < high)
			{
				auto idx = Partition(values, lt, low, high);

				Recurse(values, lt, low, idx - 1);

				Recurse(values, lt, idx + 1, high);
			}
		}
	};

	if (Int n = values.size) Impl::Recurse(values, lt, 0, n - 1);
}

template <class COMPARE, class TYPE> inline void Sort(ArrayRegion <TYPE> values)
{
	constexpr auto lt = &COMPARE::template lt<TYPE, TYPE>;

	Sort(values, lt);
}

REFLEX_END

template <class COMPARE, class ARRAY> REFLEX_INLINE void Reflex::Sort(ARRAY & values)
{
	Detail::Sort<COMPARE>(ToRegion(values));
}

template <class ARRAY, class LT> REFLEX_INLINE void Reflex::Sort(ARRAY & values, LT lt)
{
	Detail::Sort(ToRegion(values), lt);
}

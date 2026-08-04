#pragma once

#include "../detail/iterate.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE> auto ReverseIterate(TYPE && iterable);

}




//
//impl

template <class TYPE> inline auto Reflex::ReverseIterate(TYPE && iterable)
{ 
	static_assert(std::is_lvalue_reference_v<TYPE>, "ReverseIterate cannot be used with temporaries");

	return Detail::RangeHolder(iterable.rbegin(), iterable.rend()); 
}

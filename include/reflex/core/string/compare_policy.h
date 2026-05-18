#pragma once

#include "detail/compare.h"




//
//Primary API

namespace Reflex
{

	using CaseSensitive = StandardCompare;

	struct CaseInsensitive;

}




//
//CaseInsensitive

struct Reflex::CaseInsensitive
{
	template <class A, class B> static bool eq(const A & a, const B & b);

	template <class A, class B> static bool lt(const A & a, const B & b);
};




//
//impl

template <class A, class B> REFLEX_INLINE bool Reflex::CaseInsensitive::eq(const A & a, const B & b)
{
	auto [a_data, a_size] = ToView(a); 
	auto [b_data, b_size] = ToView(b);
	
	return Detail::StringEquals<true>(a_data, a_size, b_data, b_size);
}

template <class A, class B> REFLEX_INLINE bool Reflex::CaseInsensitive::lt(const A & a, const B & b)
{
	auto [a_data, a_size] = ToView(a);
	auto [b_data, b_size] = ToView(b);
	
	return Detail::StringLessThan<true>(a_data, a_size, b_data, b_size);
}

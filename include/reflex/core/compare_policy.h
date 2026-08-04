#pragma once

#include "tuple.h"




//
//Primary API

namespace Reflex
{

	struct StandardCompare;

	struct KeyCompare;

	template <auto MEMBER> struct FieldCompare;

}




//
//StandardCompare

struct Reflex::StandardCompare
{
	template <class A, class B> static bool eq(const A & a, const B & b)
	{
		//if constexpr (kIsType<A,B>)
		//{
		//	return Detail::template Evaluator<A>::eq(a, b);
		//}
		//else
		{
			return a == b;
		}
	}

	template <class A, class B> static bool lt(const A & a, const B & b) { return a < b; }
};




//
//KeyCompare

struct Reflex::KeyCompare
{
	template <class A, class B> static bool eq(const A & a, const B & b) { return a.a == b; }

	template <class A, class B> static bool lt(const A & a, const B & b) { return a.a < b; }
};




//
//FieldCompare

template <auto MEMBER>
struct Reflex::FieldCompare
{
	template <class OBJ, class VALUE>
	static bool eq(OBJ && obj, VALUE && value)
	{
		REFLEX_STATIC_ASSERT(std::is_member_object_pointer_v<decltype(MEMBER)>);

		return std::forward<OBJ>(obj).*MEMBER == std::forward<VALUE>(value);
	}
};

#pragma once

#include "tuple.h"




//
//Primary API

namespace Reflex
{

	struct StandardCompare;

	struct KeyCompare;

	template <auto MEMBER, class POLICY = StandardCompare> struct FieldCompare;

}




//
//StandardCompare

struct Reflex::StandardCompare
{
	template <class A, class B> static bool eq(const A & a, const B & b) { return a == b; }

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

template <auto MEMBER, class POLICY>
struct Reflex::FieldCompare
{
	template <class OBJ, class VALUE> static bool eq(OBJ && obj, VALUE && value)
	{
		REFLEX_STATIC_ASSERT(std::is_member_object_pointer_v<decltype(MEMBER)>);

		return POLICY::eq(std::forward<OBJ>(obj).*MEMBER, std::forward<VALUE>(value));
	}
};

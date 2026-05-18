#pragma once

#include "traits.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE> using NonPointerT = typename std::remove_pointer<TYPE>::type;

	template <class TYPE> using NonRefT = typename std::remove_reference<TYPE>::type;

	template <class TYPE> using NonConstT = typename std::remove_const<TYPE>::type;


	template <bool VALUE, class T, class F> struct Selector { using type = F; };

	template <class T, class F> struct Selector <true,T,F>;

	template <bool VALUE, class T, class F> using ConditionalType = typename Selector<VALUE,T,F>::type;


	template <bool B0, bool B1 = false, bool B2 = false, bool B3 = false, bool B4 = false, bool B5 = false, bool B6 = false, bool B7 = false> struct Bits;

}




//
//impl

template <class T, class F> struct Reflex::Selector <true,T,F> { using type = T; };

template <bool B0, bool B1, bool B2, bool B3, bool B4, bool B5, bool B6, bool B7>
struct Reflex::Bits
{
	static constexpr UInt8 value = (B0 | (B1 << 1) | (B2 << 2) | (B3 << 3) | (B4 << 4) | (B5 << 5) | (B6 << 6) | (B7 << 7));
};

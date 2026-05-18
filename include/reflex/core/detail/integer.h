#pragma once

#include "../preprocessor.h"




//
//Detail

namespace Reflex::Detail
{

	template <int SIZE> struct UIntOfSize {};

	template <> struct UIntOfSize <4> { using type = unsigned int; };

	template <> struct UIntOfSize <8> { using type = unsigned long long; };


	template <int SIZE> struct IntOfSize {};

	template <> struct IntOfSize <4> { using type = signed int; };

	template <> struct IntOfSize <8> { using type = signed long long; };

}

REFLEX_STATIC_ASSERT(sizeof(Reflex::Detail::UIntOfSize<4>::type) == 4);
REFLEX_STATIC_ASSERT(sizeof(Reflex::Detail::UIntOfSize<8>::type) == 8);
REFLEX_STATIC_ASSERT(sizeof(Reflex::Detail::IntOfSize<4>::type) == 4);
REFLEX_STATIC_ASSERT(sizeof(Reflex::Detail::IntOfSize<8>::type) == 8);
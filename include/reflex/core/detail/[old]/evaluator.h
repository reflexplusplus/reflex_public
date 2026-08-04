#pragma once

#include "../meta/auxtypes.h"
#include "../functions/cast.h"
#include "../functions/memory.h"




//
//Detail

namespace Reflex::Detail
{

	template <class TYPE> struct StandardEvaluator;

	template <class TYPE, UInt SIZE> struct RawEvaluator;

	template <class TYPE> using Evaluator = ConditionalType < (kIsClass<TYPE> && kIsRawComparable<TYPE>), RawEvaluator <TYPE, sizeof(TYPE)>, StandardEvaluator <TYPE> >;

}




//
//Detail::StandardEvaluator

template <class TYPE>
struct Reflex::Detail::StandardEvaluator
{
	REFLEX_INLINE static bool eq(const TYPE & a, const TYPE & b)
	{
		return a == b;
	}

	REFLEX_INLINE static bool ineq(const TYPE & a, const TYPE & b)
	{
		return a != b;
	}

	REFLEX_INLINE static bool lt(const TYPE & a, const TYPE & b)
	{
		return a < b;
	}
};




//
//Detail::RawEvaluator

template <class TYPE, Reflex::UInt SIZE>
struct Reflex::Detail::RawEvaluator
{
	REFLEX_INLINE static bool eq(const TYPE & a, const TYPE & b)
	{
		return MemCompare(&a, &b, sizeof(TYPE));
	}

	REFLEX_INLINE static bool ineq(const TYPE & a, const TYPE & b)
	{
		return !MemCompare(&a, &b, sizeof(TYPE));
	}
};

template <class TYPE>
struct Reflex::Detail::RawEvaluator <TYPE,1>
{
	REFLEX_INLINE static bool eq(const TYPE & a, const TYPE & b)
	{
		return Reinterpret<UInt8>(a) == Reinterpret<UInt8>(b);
	}

	REFLEX_INLINE static bool ineq(const TYPE & a, const TYPE & b)
	{
		return Reinterpret<UInt8>(a) != Reinterpret<UInt8>(b);
	}
};

template <class TYPE>
struct Reflex::Detail::RawEvaluator <TYPE,4>
{
	REFLEX_INLINE static bool eq(const TYPE & a, const TYPE & b)
	{
		return Reinterpret<UInt32>(a) == Reinterpret<UInt32>(b);
	}

	REFLEX_INLINE static bool ineq(const TYPE & a, const TYPE & b)
	{
		return Reinterpret<UInt32>(a) != Reinterpret<UInt32>(b);
	}
};

template <class TYPE>
struct Reflex::Detail::RawEvaluator <TYPE,8>
{
	REFLEX_INLINE static bool eq(const TYPE & a, const TYPE & b)
	{
		return Reinterpret<UInt64>(a) == Reinterpret<UInt64>(b);
	}

	REFLEX_INLINE static bool ineq(const TYPE & a, const TYPE & b)
	{
		return Reinterpret<UInt64>(a) != Reinterpret<UInt64>(b);
	}
};

#pragma once

#include "../meta/traits.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE> bool SetFiltered(TYPE & value, const TYPE & set);

	template <class TYPE> TYPE SetDelta(TYPE & value, const TYPE & set);

	template <class TYPE> TYPE Poll(TYPE & value);


	template <class TYPE> void Swap(TYPE & a, TYPE & b);


	bool True(bool value);


	bool True(Float value);

	bool True(Float64 value);


	bool True(UInt8 value);

	bool True(UInt16 value);

	bool True(UInt32 value);

	bool True(UInt64 value);


	bool True(Int8 value);

	bool True(Int16 value);

	bool True(Int32 value);

	bool True(Int64 value);


	template <class TYPE> bool True(const TYPE & value);


	template <class TYPE> bool True(TYPE * value);

	template <class TYPE> bool True(const TYPE * value);


	bool Not(bool a);

	template <class TYPE> bool Not(const TYPE & value);


	template <typename FIRST, typename ... VARGS> bool And(const FIRST & a, VARGS && ... args);

	template <typename FIRST, typename ... VARGS> bool Or(const FIRST & a, VARGS && ... args);

}




//
//impl

REFLEX_NS(Reflex::Detail)

template <class TYPE, bool SCALAR>
struct ToBool
{
	REFLEX_INLINE static bool Call(const TYPE & value)
	{
		return value;
	}
};

template <class TYPE>
struct ToBool <TYPE, true>
{
	REFLEX_INLINE static bool Call(const TYPE & value)
	{
		return value != 0;
	}
};

REFLEX_END

REFLEX_NS(Reflex)

template <typename TYPE> REFLEX_INLINE bool And(const TYPE & a)
{
	return True(a);
}

template <typename TYPE> REFLEX_INLINE bool Or(const TYPE & a)
{
	return True(a);
}

REFLEX_END

REFLEX_INLINE bool Reflex::True(bool value)
{
	return value;
}

REFLEX_INLINE bool Reflex::True(UInt8 value)
{
	return value != 0;
}

REFLEX_INLINE bool Reflex::True(UInt16 value)
{
	return value != 0;
}

REFLEX_INLINE bool Reflex::True(UInt32 value)
{
	return value != 0;
}

REFLEX_INLINE bool Reflex::True(UInt64 value)
{
	return value != 0;
}

REFLEX_INLINE bool Reflex::True(Int8 value)
{
	return value != 0;
}

REFLEX_INLINE bool Reflex::True(Int16 value)
{
	return value != 0;
}

REFLEX_INLINE bool Reflex::True(Int32 value)
{
	return value != 0;
}

REFLEX_INLINE bool Reflex::True(Int64 value)
{
	return value != 0;
}

REFLEX_INLINE bool Reflex::True(Float value)
{
	return value != 0.0f;
}

REFLEX_INLINE bool Reflex::True(Float64 value)
{
	return value != 0.0;
}

template <class TYPE> REFLEX_INLINE bool Reflex::True(const TYPE & value)
{
	if constexpr (kIsBoolCastable<TYPE>)
	{
		return bool(value);
	}
	else
	{
		return Detail::ToBool<TYPE,kIsScalar<TYPE>>::Call(value);
	}
}

template <class TYPE> REFLEX_INLINE bool Reflex::True(TYPE * value)
{
	return value != 0;
}

template <class TYPE> REFLEX_INLINE bool Reflex::True(const TYPE * value)
{
	return value != 0;
}

REFLEX_INLINE bool Reflex::Not(bool a)
{
	return !a;
}

template <class TYPE> REFLEX_INLINE bool Reflex::Not(const TYPE & value)
{
	return !True(value);
}

template <typename FIRST, typename... VARGS> REFLEX_INLINE bool Reflex::And(const FIRST & a, VARGS &&... args)
{
	return a && And(args...);
}

template <typename FIRST, typename... VARGS> REFLEX_INLINE bool Reflex::Or(const FIRST & a, VARGS &&... args)
{
	return a || Or(args...);
}

template <class TYPE> REFLEX_INLINE void Reflex::Swap(TYPE & a, TYPE & b)
{
	TYPE t = a;

	a = b;

	b = t;
}

template <class TYPE> REFLEX_INLINE bool Reflex::SetFiltered(TYPE & value, const TYPE & set)
{
	if (value != set)
	{
		value = set;

		return true;
	}

	return false;
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::SetDelta(TYPE & value, const TYPE & set)
{
	auto delta = set - value;

	value = set;

	return delta;
}

template <class TYPE> inline TYPE Reflex::Poll(TYPE & value)
{
	auto rtn = value;

	value = {};

	return rtn;
}

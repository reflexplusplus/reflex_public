#pragma once

#include "bool_v4.h"




//
//Primary API

namespace Reflex::SIMD
{

	using IntV4 = TypeV4 <Int32>;


	BoolV4 operator==(const IntV4 & a, const IntV4 & b);

	BoolV4 operator!=(const IntV4 & a, const IntV4 & b);

	BoolV4 operator>(const IntV4 & a, const IntV4 & b);

	BoolV4 operator<(const IntV4 & a, const IntV4 & b);


	IntV4 operator+(const IntV4 & a, const IntV4 & b);

	IntV4 operator-(const IntV4 & a, const IntV4 & b);

	IntV4 operator*(const IntV4 & a, const IntV4 & b);


	IntV4 operator&(const IntV4 & a, const IntV4 & b);

	IntV4 operator|(const IntV4 & a, const IntV4 & b);


	IntV4 Abs(const IntV4 & value);

	//IntV4 Sign(const IntV4 & value);

	//IntV4 Invert(const IntV4 & value);


	IntV4 Min(const IntV4 & a, const IntV4 & b);

	IntV4 Max(const IntV4 & a, const IntV4 & b);


	//IntV4 Modulo(const IntV4 & a, const IntV4 & b);

}

REFLEX_SET_TRAIT(SIMD::IntV4, IsRawComparable);




//
//impl

REFLEX_NS(Reflex::SIMD)

extern const IntV4 kiZero;

extern const IntV4 kiOne;

REFLEX_END

REFLEX_INLINE Reflex::SIMD::BoolV4 Reflex::SIMD::operator==(const IntV4 & a, const IntV4 & b)
{
	return Reinterpret<BoolV4>(REFLEX_EQ_I32_X4(a, b));
}

REFLEX_INLINE Reflex::SIMD::BoolV4 Reflex::SIMD::operator!=(const IntV4 & a, const IntV4 & b)
{
	return (a == b) == kFalseV4; //WRONG?
}

REFLEX_INLINE Reflex::SIMD::BoolV4 Reflex::SIMD::operator>(const IntV4 & a, const IntV4 & b)
{
	return Reinterpret<BoolV4>(REFLEX_GT_I32_X4(a, b));
}

REFLEX_INLINE Reflex::SIMD::BoolV4 Reflex::SIMD::operator<(const IntV4 & a, const IntV4 & b)
{
	return Reinterpret<BoolV4>(REFLEX_LT_I32_X4(a, b));
}

REFLEX_INLINE Reflex::SIMD::IntV4 Reflex::SIMD::operator+(const IntV4 & a, const IntV4 & b)
{
	return Reinterpret<IntV4>(REFLEX_ADD_I32_X4(a, b));
}

REFLEX_INLINE Reflex::SIMD::IntV4 Reflex::SIMD::operator-(const IntV4 & a, const IntV4 & b)
{
	return Reinterpret<IntV4>(REFLEX_SUB_I32_X4(a, b));
}

REFLEX_INLINE Reflex::SIMD::IntV4 Reflex::SIMD::operator*(const IntV4 & a, const IntV4 & b)
{
	return Reinterpret<IntV4>(REFLEX_MUL_I32_X4(a, b));
}

REFLEX_INLINE Reflex::SIMD::IntV4 Reflex::SIMD::operator&(const IntV4 & a, const IntV4 & b)
{
	return Reinterpret<IntV4>(REFLEX_AND_I32_X4(a, b));
}

REFLEX_INLINE Reflex::SIMD::IntV4 Reflex::SIMD::operator|(const IntV4 & a, const IntV4 & b)
{
	return Reinterpret<IntV4>(REFLEX_OR_I32_X4(a, b));
}

REFLEX_INLINE Reflex::SIMD::IntV4 Reflex::SIMD::Abs(const IntV4 & value)
{
	BoolV4 negative = (value < kiZero);

	IntV4 inverted = Reinterpret<IntV4>(REFLEX_XOR_I32_X4(value, negative));

	return Reinterpret<IntV4>(inverted - Reinterpret<IntV4>(negative));
}

REFLEX_INLINE Reflex::SIMD::IntV4 Reflex::SIMD::Min(const IntV4 & a, const IntV4 & b)
{
	return Select(a < b, a, b);
}

REFLEX_INLINE Reflex::SIMD::IntV4 Reflex::SIMD::Max(const IntV4 & a, const IntV4 & b)
{
	return Select(a > b, a, b);
}

#pragma once

#include "type_v4.h"




//
//Primary API

namespace Reflex::SIMD
{

	using BoolV4 = TypeV4 <Boolean>;


	inline Boolean ToBool(bool value) { return value ? kBooleanTrue : kBooleanFalse; }


	Int GetFlags(const BoolV4 & value);

	Int Count(const BoolV4 & value);

	bool Empty(const BoolV4 & value);

	bool Any(const BoolV4 & value);

	bool Full(const BoolV4 & value);

	UInt GetFree(const BoolV4 & value);


	BoolV4 operator==(const BoolV4 & a, const BoolV4 & b);

	BoolV4 operator!=(const BoolV4 & a, const BoolV4 & b);


	BoolV4 Not(const BoolV4 & a);

	BoolV4 Or(const BoolV4 & a, const BoolV4 & b);

	BoolV4 And(const BoolV4 & a, const BoolV4 & b);


	template <class TYPE> TYPE Select(const BoolV4 & mask, const TYPE & value);

	template <class TYPE> TYPE SelectNot(const BoolV4 & mask, const TYPE & value);

	template <class TYPE> TYPE Select(const BoolV4 & mask, const TYPE & t, const TYPE & f);


	extern const BoolV4 kTrueV4;

	extern const BoolV4 kFalseV4;

}




//
//impl

REFLEX_NS(Reflex::SIMD::Detail)

extern const Int kCount[16];

extern const UInt kFree[16];

REFLEX_END

REFLEX_INLINE Reflex::Int Reflex::SIMD::GetFlags(const BoolV4 & value)
{
	return REFLEX_I32_X4_TO_I32_MASK(Reinterpret<System::Int32x4>(value.data));
}

REFLEX_INLINE Reflex::Int Reflex::SIMD::Count(const BoolV4 & value)
{
	Int sum = Detail::kCount[GetFlags(value)];

	return sum;
}

REFLEX_INLINE bool Reflex::SIMD::Empty(const BoolV4 & value)
{
	return GetFlags(value) == 0;
}

REFLEX_INLINE bool Reflex::SIMD::Any(const BoolV4 & value)
{
	return GetFlags(value) != 0;
}

REFLEX_INLINE bool Reflex::SIMD::Full(const BoolV4 & value)
{
	return GetFlags(value) == 15;
}

REFLEX_INLINE Reflex::UInt Reflex::SIMD::GetFree(const BoolV4 & value)
{
	return Detail::kFree[GetFlags(value)];
}

REFLEX_INLINE Reflex::SIMD::BoolV4 Reflex::SIMD::operator==(const BoolV4 & a, const BoolV4 & b)
{
	return Reinterpret<BoolV4>(REFLEX_EQ_I32_X4(Reinterpret<System::Int32x4>(a.data), Reinterpret<System::Int32x4>(b.data)));
}

REFLEX_INLINE Reflex::SIMD::BoolV4 Reflex::SIMD::operator!=(const BoolV4 & a, const BoolV4 & b)
{
	return (a == b) == kFalseV4;
}

REFLEX_INLINE Reflex::SIMD::BoolV4 Reflex::SIMD::Not(const BoolV4 & a)
{
	return a == kFalseV4;
}

REFLEX_INLINE Reflex::SIMD::BoolV4 Reflex::SIMD::Or(const BoolV4 & a, const BoolV4 & b)
{
	return Reinterpret<BoolV4>(REFLEX_OR_I32_X4(a.data, b.data));
}

REFLEX_INLINE Reflex::SIMD::BoolV4 Reflex::SIMD::And(const BoolV4 & a, const BoolV4 & b)
{
	return Reinterpret<BoolV4>(REFLEX_AND_I32_X4(a.data, b.data));
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::SIMD::Select(const BoolV4 & mask, const TYPE & value)
{
	using VectorType = PlatformVectorType <typename TYPE::SingleType,4>;

	return Reinterpret<TYPE>(VectorType::And(value, Reinterpret<TYPE>(mask)));
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::SIMD::SelectNot(const BoolV4 & mask, const TYPE & value)
{
	using VectorType = PlatformVectorType <typename TYPE::SingleType,4>;

	return Reinterpret<TYPE>(VectorType::AndNot(Reinterpret<TYPE>(mask), value));
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::SIMD::Select(const BoolV4 & mask, const TYPE & t, const TYPE & f)
{
	using VectorType = PlatformVectorType <typename TYPE::SingleType,4>;

	auto a = VectorType::And(t, Reinterpret<TYPE>(mask));

	auto b = VectorType::AndNot(Reinterpret<TYPE>(mask), f);

	return Reinterpret<TYPE>(VectorType::Or(a, b));
}

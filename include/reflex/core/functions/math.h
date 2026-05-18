#pragma once

#include "detail/math.h"




//
//declarations

namespace Reflex
{

	Float32 ToFloat32(Int32 value);

	Float32 ToFloat32(Int64 value);

	Float32 ToFloat32(UInt32 value);

	Float32 ToFloat32(UInt64 value);


	Int32 ToInt32(Float32 value);

	Int32 ToInt32(Float64 value);


	Int32 Truncate(Float32 value);

	Int32 Truncate(Float64 value);


	template <class FLOAT> FLOAT RoundNearest(FLOAT value);

	template <class FLOAT> FLOAT RoundDown(FLOAT value);

	template <class FLOAT> FLOAT RoundUp(FLOAT value);


	template <class TYPE> TYPE Quantise(TYPE value, TYPE step);

	template <class TYPE> TYPE QuantiseDown(TYPE value, TYPE step);

	template <class TYPE> TYPE QuantiseUp(TYPE value, TYPE step);


	template <class TYPE> TYPE Modulo(TYPE a, TYPE b);


	template <class TYPE> TYPE Sign(TYPE value);

	template <class TYPE> TYPE Abs(TYPE value);


	template <class TYPE> TYPE SquareRoot(TYPE value);


	template <class TYPE> TYPE MulAdd(TYPE a, TYPE b, TYPE c);


	template <class TYPE> TYPE Square(TYPE value);

	template <class TYPE> TYPE Cube(TYPE value);

	template <class TYPE> TYPE Quartic(TYPE value);


	template <class TYPE> TYPE Log(TYPE x);

	template <class TYPE> TYPE Exp(TYPE x);


	template <class TYPE> TYPE Log2(TYPE x);

	template <class TYPE> TYPE Exp2(TYPE x);


	template <class TYPE> TYPE Pow(TYPE x, TYPE y);


	template <class TYPE> TYPE RoundUpPow2(TYPE value, TYPE start = 1);


	template <class TYPE> TYPE Sin(TYPE value);

	template <class TYPE> TYPE Cos(TYPE value);


	template <class TYPE> TYPE Min(TYPE a, TYPE b);

	template <class TYPE> TYPE Max(TYPE a, TYPE b);


	template <class TYPE> TYPE Clip(const TYPE & value, const TYPE & min, const TYPE & max);


	template <class FLOAT> FLOAT Reciprocal(FLOAT value);


	template <class TYPE> bool Inside(TYPE value, TYPE start, TYPE range);

	template <class TYPE> bool Inside(TYPE * value, TYPE * start, UIntNative range);


	template <class FLOAT> FLOAT LinearInterpolate(FLOAT x, FLOAT in0, FLOAT in1);

	template <class FLOAT> FLOAT Normalise(FLOAT value, FLOAT in0, FLOAT in1);

}




//
//impl

REFLEX_INLINE Reflex::Float32 Reflex::ToFloat32(Int32 value)
{
	return Float32(value);
}

REFLEX_INLINE Reflex::Float32 Reflex::ToFloat32(Int64 value)
{
	return Float32(value);
}

REFLEX_INLINE Reflex::Float32 Reflex::ToFloat32(UInt32 value)
{
	return Float32(value);
}

REFLEX_INLINE Reflex::Float32 Reflex::ToFloat32(UInt64 value)
{
	return Float32(value);
}

REFLEX_INLINE Reflex::Int32 Reflex::ToInt32(Float32 value)
{
	return Detail::FloatToInt32(value);
}

REFLEX_INLINE Reflex::Int32 Reflex::ToInt32(Float64 value)
{
	return Detail::FloatToInt32(value);
}

REFLEX_INLINE Reflex::Int32 Reflex::Truncate(Float32 value)
{
	return Detail::Truncate(value);
}

REFLEX_INLINE Reflex::Int32 Reflex::Truncate(Float64 value)
{
	return Detail::Truncate(value);
}

template <class FLOAT> REFLEX_INLINE FLOAT Reflex::RoundNearest(FLOAT value)
{
	return Detail::RoundNearest(value);
}

template <class FLOAT> REFLEX_INLINE FLOAT Reflex::RoundDown(FLOAT value)
{
	return Detail::RoundDown(value);
}

template <class FLOAT> REFLEX_INLINE FLOAT Reflex::RoundUp(FLOAT value)
{
	return Detail::RoundUp(value);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Quantise(TYPE value, TYPE step)
{
	return Detail::Quantise(value, step);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::QuantiseDown(TYPE value, TYPE step)
{
	return Detail::QuantiseDown(value, step);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::QuantiseUp(TYPE value, TYPE step)
{
	return Detail::QuantiseUp(value, step);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Modulo(TYPE a, TYPE b)
{
	return Detail::Modulo(a, b);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Sign(TYPE value)
{
	using Constants = Detail::Constants <TYPE>;

	return value < Constants::kZero ? Constants::kMinusOne : Constants::kOne;
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Abs(TYPE value)
{
	return Detail::Abs(value);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::SquareRoot(TYPE value)
{
	return Detail::SquareRoot(value);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::MulAdd(TYPE a, TYPE b, TYPE c)
{
	if constexpr (IsType<TYPE,Float32>::value)
	{
		return fma(a, b, c);
	}
	else if constexpr (IsType<TYPE, Float64>::value)
	{
		return fma(a, b, c);
	}
	else
	{
		REFLEX_STATIC_ASSERT(sizeof(TYPE) == 0);
	}
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Square(TYPE value)
{
	return value * value;
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Cube(TYPE value)
{
	return Square(value) * value;
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Quartic(TYPE value)
{
	return Square(Square(value));
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Log(TYPE value)
{
	REFLEX_ASSERT(value);

	return Detail::Log(value);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Exp(TYPE value)
{
	return Detail::Exp(value);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Log2(TYPE value)
{
	REFLEX_ASSERT(value);

	return Detail::Log2(value);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Exp2(TYPE value)
{
	return Detail::Exp2(value);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Pow(TYPE x, TYPE y)
{
	return Detail::Pow(x, y);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Sin(TYPE value)
{
	return Detail::Sin(value);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Cos(TYPE value)
{
	return Detail::Cos(value);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::RoundUpPow2(TYPE value, TYPE start)
{
	TYPE v = start;

	while (v < value) Detail::Double(v);

	return v;
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Min(TYPE a, TYPE b)
{
	if (a > b)
	{
		return b;
	}

	return a;
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Max(TYPE a, TYPE b)
{
	if (a > b)
	{
		return a;
	}

	return b;
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Clip(const TYPE & value, const TYPE & min, const TYPE & max)
{
	return Min(Max(value, min), max);
}

template <class FLOAT> REFLEX_INLINE FLOAT Reflex::Reciprocal(FLOAT value)
{
	return Detail::Reciprocal(value);
}

template <class FLOAT> REFLEX_INLINE FLOAT Reflex::LinearInterpolate(FLOAT x, FLOAT in0, FLOAT in1)
{
	return (x * (in1 - in0)) + in0;
}

template <class FLOAT> REFLEX_INLINE FLOAT Reflex::Normalise(FLOAT in, FLOAT in0, FLOAT in1)
{
	return (in - in0) * Reciprocal(in1 - in0);
}

template <class TYPE> REFLEX_INLINE bool Reflex::Inside(TYPE value, TYPE start, TYPE range)
{
	if (value < start)
	{
		return false;
	}
	else if (value < (start + range))
	{
		return true;
	}
	else
	{
		return false;
	}
}

template <class TYPE> REFLEX_INLINE bool Reflex::Inside(TYPE * value, TYPE * start, UIntNative range)
{
	if (value < start)
	{
		return false;
	}
	else if (value < (start + range))
	{
		return true;
	}
	else
	{
		return false;
	}
}

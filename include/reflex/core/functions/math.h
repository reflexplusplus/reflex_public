#pragma once

#include "../types.h"
#include "../meta/traits.h"
#include "../../system/math.h"




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


	template <class FLOAT> FLOAT RoundNearest(FLOAT value); //fast, .5 cases may round either up or down depending on platform

	template <class FLOAT> FLOAT RoundDown(FLOAT value);

	template <class FLOAT> FLOAT RoundUp(FLOAT value);

	template <class TYPE> TYPE Quantise(TYPE value, TYPE step);

	template <class TYPE> TYPE QuantiseDown(TYPE value, TYPE step);

	template <class TYPE> TYPE QuantiseUp(TYPE value, TYPE step);


	template <class TYPE> TYPE Modulo(TYPE a, TYPE b);

	template <class TYPE> TYPE Sign(TYPE value);

	template <class TYPE> TYPE Abs(TYPE value);

	template <class TYPE> TYPE MulAdd(TYPE a, TYPE b, TYPE c);

	template <class FLOAT> FLOAT Reciprocal(FLOAT value);


	template <class TYPE> TYPE SquareRoot(TYPE value);

	template <class TYPE> TYPE CubeRoot(TYPE value);

	template <class TYPE> TYPE Square(TYPE value);

	template <class TYPE> TYPE Cube(TYPE value);

	template <class TYPE> TYPE Quartic(TYPE value);

	template <class TYPE> TYPE Pow(TYPE x, TYPE y);

	template <class TYPE> TYPE RoundUpPow2(TYPE value, TYPE start = 1);


	template <class TYPE> TYPE Exp(TYPE x);

	template <class TYPE> TYPE Exp2(TYPE x);

	template <class TYPE> TYPE Log(TYPE x);

	template <class TYPE> TYPE Log2(TYPE x);


	template <class TYPE> TYPE Sin(TYPE value);

	template <class TYPE> TYPE Cos(TYPE value);

	template <class TYPE> TYPE Atan2(TYPE y, TYPE x);


	template <class TYPE> TYPE Min(TYPE a, TYPE b);

	template <class TYPE> TYPE Max(TYPE a, TYPE b);

	template <class TYPE> TYPE Clip(const TYPE & value, const TYPE & min, const TYPE & max);

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
	return System::FloatToInt32(value);
}

REFLEX_INLINE Reflex::Int32 Reflex::ToInt32(Float64 value)
{
	return System::FloatToInt32(value);
}

REFLEX_INLINE Reflex::Int32 Reflex::Truncate(Float32 value)
{
	return System::Truncate(value);
}

REFLEX_INLINE Reflex::Int32 Reflex::Truncate(Float64 value)
{
	return System::Truncate(value);
}

template <class FLOAT> REFLEX_INLINE FLOAT Reflex::RoundNearest(FLOAT value)
{
	REFLEX_STATIC_ASSERT(kIsType<FLOAT, Float32, Float64>);

	return System::RoundNearest(value);
}

template <class FLOAT> REFLEX_INLINE FLOAT Reflex::RoundDown(FLOAT value)
{
	REFLEX_STATIC_ASSERT(kIsType<FLOAT, Float32, Float64>);

	return System::RoundDown(value);
}

template <class FLOAT> REFLEX_INLINE FLOAT Reflex::RoundUp(FLOAT value)
{
	REFLEX_STATIC_ASSERT(kIsType<FLOAT, Float32, Float64>);

	return System::RoundUp(value);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Quantise(TYPE value, TYPE step)
{
	REFLEX_STATIC_ASSERT(kIsType<TYPE, Float32, Float64>);

	value = value / step;
	value = System::RoundNearest(value);
	return value * step;
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::QuantiseDown(TYPE value, TYPE step)
{
	REFLEX_STATIC_ASSERT(kIsType<TYPE, UInt32, UInt64, Int32, Int64, Float32, Float64>);

	if constexpr (kIsType<TYPE, Float32, Float64>)
	{
		value = value / step;
		value = System::RoundDown(value);
		return value * step;
	}
	else
	{
		value = value / step;
		return value * step;
	}
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::QuantiseUp(TYPE value, TYPE step)
{
	REFLEX_STATIC_ASSERT(kIsType<TYPE, UInt32, UInt64, Int32, Int64, Float32, Float64>);

	if constexpr (kIsType<TYPE, Float32, Float64>)
	{
		value = value / step;
		value = System::RoundUp(value);
		return value * step;
	}
	else
	{
		TYPE q = value / step;
		q *= step;
		if (q < value) return q + step;
		return q;
	}
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Modulo(TYPE a, TYPE b)
{
    REFLEX_STATIC_ASSERT(kIsType<TYPE, UInt32, UInt64, Int32, Int64, Float32, Float64>);

    if constexpr (kIsType<TYPE, UInt32, UInt64>)
    {
        return a % b;
    }
    else if constexpr (kIsType<TYPE, Int32, Int64>)
    {
        a = a % b;
        if (a < 0) a = b + a;
        return a;
    }
    else
    {
        TYPE round = System::RoundDown(a / b);
        a -= round * b;
        if (a < TYPE(0)) a += b;
        return a;
    }
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Sign(TYPE value)
{
	return value < TYPE(0) ? TYPE(-1) : TYPE(1);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Abs(TYPE value)
{
	REFLEX_STATIC_ASSERT(kIsType<TYPE, Float32, Float64, Int32, Int64>);
	
	return System::Abs(value);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::MulAdd(TYPE a, TYPE b, TYPE c)
{
	REFLEX_STATIC_ASSERT(kIsType<TYPE, Float32, Float64>);

	return System::MulAdd(a, b, c);
}

template <class FLOAT> REFLEX_INLINE FLOAT Reflex::Reciprocal(FLOAT value)
{
	return FLOAT(1.0) / value;
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::SquareRoot(TYPE value)
{
	REFLEX_STATIC_ASSERT(kIsType<TYPE, UInt32, Float32, Float64>);
	
	if constexpr (kIsType<TYPE, UInt32>) 
	{
		UInt32 place = UInt32(1 << (sizeof (UInt32) * 8 - 2)); // calculated by precompiler = same runtime as: place = 0x40000000

		while (place > value) place >>= 2; //place /= 4;

		UInt32 root = 0;

		while (place)
		{
			if (value >= root + place)
			{
				value -= root + place;

				root += (place << 1);	//place * 2;
			}

			root >>= 1; //root /= 2;

			place >>= 2; //place /= 4;;
		}

		return root;
	}
	else 
	{
		return System::SquareRoot(value);
	}
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::CubeRoot(TYPE value)
{
	REFLEX_STATIC_ASSERT(kIsType<TYPE, Float32, Float64>);
	
	return System::CubeRoot(value);
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

template <class TYPE> REFLEX_INLINE TYPE Reflex::Pow(TYPE x, TYPE y)
{
	REFLEX_STATIC_ASSERT(kIsType<TYPE, Float32, Float64>);
	
	return System::Pow(x, y);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::RoundUpPow2(TYPE value, TYPE start)
{
	TYPE v = start;

	while (v < value)
	{
		if constexpr (kIsType<TYPE, Float32, Float64>) v *= TYPE(2);
		else v <<= 1;
	}

	return v;
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Exp(TYPE value)
{
	REFLEX_STATIC_ASSERT(kIsType<TYPE, Float32, Float64>);

	return System::Exp(value);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Exp2(TYPE value)
{
	REFLEX_STATIC_ASSERT(kIsType<TYPE, Float32, Float64>);
	
	return System::Exp2(value);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Log(TYPE value)
{
	REFLEX_STATIC_ASSERT(kIsType<TYPE, Float32, Float64>);
	REFLEX_ASSERT(value);

	return System::Log(value);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Log2(TYPE value)
{
	REFLEX_STATIC_ASSERT(kIsType<TYPE, Float32, Float64>);
	REFLEX_ASSERT(value);

	return System::Log2(value);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Sin(TYPE value)
{
	REFLEX_STATIC_ASSERT(kIsType<TYPE, Float32, Float64>);
	
	return System::Sin(value);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Cos(TYPE value)
{
	REFLEX_STATIC_ASSERT(kIsType<TYPE, Float32, Float64>);
	
	return System::Cos(value);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::Atan2(TYPE y, TYPE x)
{
	REFLEX_STATIC_ASSERT(kIsType<TYPE, Float32, Float64>);
	
	return System::Atan2(y, x);
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

template <class FLOAT> REFLEX_INLINE FLOAT Reflex::LinearInterpolate(FLOAT x, FLOAT in0, FLOAT in1)
{
	return (x * (in1 - in0)) + in0;
}

template <class FLOAT> REFLEX_INLINE FLOAT Reflex::Normalise(FLOAT in, FLOAT in0, FLOAT in1)
{
	return (in - in0) * Reciprocal(in1 - in0);
}

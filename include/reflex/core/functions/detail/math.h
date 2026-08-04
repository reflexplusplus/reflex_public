#pragma once

#include "../../types.h"




//
//declarations

REFLEX_NS(Reflex::Detail)

Int32 FloatToInt32(Float32 value);

Int32 FloatToInt32(Float64 value);


Int32 Truncate(Float32 value);

Int32 Truncate(Float64 value);


Float32 RoundNearest(Float32 value);	//fast, .5 cases may round either up or down depending on platform

Float64 RoundNearest(Float64 value);	//fast, .5 cases may round either up or down depending on platform
 

Float32 RoundDown(Float32 value);

Float64 RoundDown(Float64 value);


Float32 RoundUp(Float32 value);

Float64 RoundUp(Float64 value);


Float32 Quantise(Float32 value, Float32 step);

Float64 Quantise(Float64 value, Float64 step);


UInt32 QuantiseDown(UInt32 value, UInt32 step);

UInt64 QuantiseDown(UInt64 value, UInt64 step);

Int32 QuantiseDown(Int32 value, Int32 step);

Int64 QuantiseDown(Int64 value, Int64 step);

Float32 QuantiseDown(Float32 value, Float32 step);

Float64 QuantiseDown(Float64 value, Float64 step);


UInt32 QuantiseUp(UInt32 value, UInt32 step);

UInt64 QuantiseUp(UInt64 value, UInt64 step);

Int32 QuantiseUp(Int32 value, Int32 step);

Int64 QuantiseUp(Int64 value, Int64 step);

Float32 QuantiseUp(Float32 value, Float32 step);

Float64 QuantiseUp(Float64 value, Float64 step);


UInt32 Modulo(UInt32 a, UInt32 b);

UInt64 Modulo(UInt64 a, UInt64 b);

Int32 Modulo(Int32 a, Int32 b);

Int64 Modulo(Int64 a, Int64 b);

Float32 Modulo(Float32 a, Float32 b);

Float64 Modulo(Float64 a, Float64 b);


Int32 Abs(Int32 value);

Int64 Abs(Int64 value);

Float32 Abs(Float32 value);

Float64 Abs(Float64 value);


UInt32 SquareRoot(UInt32 value);

Float32 SquareRoot(Float32 value);

Float64 SquareRoot(Float64 value);


Float32 Log(Float32 x);

Float64 Log(Float64 x);


Float32 Log2(Float32 x);

Float64 Log2(Float64 x);


Float32 Exp(Float32 x);

Float64 Exp(Float64 x);


Float32 Exp2(Float32 x);

Float64 Exp2(Float64 x);


Float32 Pow(Float32 x, Float32 y);

Float64 Pow(Float64 x, Float64 y);


Float32 Sin(Float32 value);

Float64 Sin(Float64 value);


Float32 Cos(Float32 value);

Float64 Cos(Float64 value);


Float32 Reciprocal(Float32 value);

Float64 Reciprocal(Float64 value);


Float32 LinearInterpolate(Float32 x, Float32 in0, Float32 in1);

Float64 LinearInterpolate(Float64 x, Float64 in0, Float64 in1);


Float32 Normalise(Float32 value, Float32 in0, Float32 in1);

Float64 Normalise(Float64 value, Float64 in0, Float64 in1);


template <class INTEGER> REFLEX_INLINE void Double(INTEGER & i) { i <<= 1; }

REFLEX_INLINE void Double(Float32 & f32) { f32 *= 2.0f; }

REFLEX_INLINE void Double(Float64 & f64) { f64 *= 2.0; }


template <class TYPE>
struct Constants
{
	static constexpr TYPE kMinusOne = TYPE(-1);

	static constexpr TYPE kZero = TYPE(0);

	static constexpr TYPE kOne = TYPE(1);
};

REFLEX_END




//
//impl

REFLEX_INLINE Reflex::Int32 Reflex::Detail::FloatToInt32(Float32 value)
{
	return Reinterpret<Int32>(lrintf(value));
}

REFLEX_INLINE Reflex::Int32 Reflex::Detail::FloatToInt32(Float64 value)
{
	return Reinterpret<Int32>(lrint(value));
}

REFLEX_INLINE Reflex::Int32 Reflex::Detail::Truncate(Float32 value)
{
	return Int32(value);
}

REFLEX_INLINE Reflex::Int32 Reflex::Detail::Truncate(Float64 value)
{
	return Int32(value);
}

REFLEX_INLINE Reflex::Float32 Reflex::Detail::RoundNearest(Float32 value)
{
#ifdef REFLEX_INTEL
	return Float32(_mm_cvtss_si32(_mm_set1_ps(value)));	//last tested 2024 as 10x faster
#else
	return round(value);
#endif
}

REFLEX_INLINE Reflex::Float64 Reflex::Detail::RoundNearest(Float64 value)
{
#if defined (REFLEX_INTEL) && REFLEX_64BIT
	return Float64(_mm_cvtsd_si64(_mm_set_sd(value)));	//last tested 2024 as 10x faster
#else
	return round(value);
#endif
}

REFLEX_INLINE Reflex::Float32 Reflex::Detail::RoundDown(Float32 value)
{
	return floor(value);

	//__m128 f = _mm_set1_ps(value);

	//__m128i i = _mm_cvttps_epi32(f);

	//return _mm_cvtss_f32(_mm_cvtepi32_ps(i));
}

REFLEX_INLINE Reflex::Float64 Reflex::Detail::RoundDown(Float64 value)
{
	return floor(value);

	//__m128d d =_mm_set1_pd(value);

	//__m128i i = _mm_cvttpd_epi32(d);

	//_mm_storel_pd( &value, _mm_cvtepi32_pd(i));
	//
	//return value;
}

REFLEX_INLINE Reflex::Float32 Reflex::Detail::RoundUp(Float32 value)
{
	return ceil(value);

	//__m128 f = _mm_set1_ps(value);

	//__m128i i = _mm_cvttps_epi32(f);

	//return _mm_cvtss_f32(_mm_cvtepi32_ps(i));
}

REFLEX_INLINE Reflex::Float64 Reflex::Detail::RoundUp(Float64 value)
{
	return ceil(value);

	//__m128d d =_mm_set1_pd(value);

	//__m128i i = _mm_cvttpd_epi32(d);
	//_mm_storel_pd( &value, _mm_cvtepi32_pd(i));
	//
	//return value;
}

REFLEX_INLINE Reflex::Float32 Reflex::Detail::Quantise(Float32 value, Float32 step)
{
	value = value / step;

	value = RoundNearest(value);

	return value * step;
}

REFLEX_INLINE Reflex::Float64 Reflex::Detail::Quantise(Float64 value, Float64 step)
{
	value = value / step;

	value = RoundNearest(value);

	return value * step;
}

REFLEX_INLINE Reflex::UInt32 Reflex::Detail::QuantiseDown(UInt32 value, UInt32 step)
{
	value = value / step;

	return value * step;
}

REFLEX_INLINE Reflex::UInt64 Reflex::Detail::QuantiseDown(UInt64 value, UInt64 step)
{
	value = value / step;

	return value * step;
}

REFLEX_INLINE Reflex::Int32 Reflex::Detail::QuantiseDown(Int32 value, Int32 step)
{
	value = value / step;

	return value * step;
}

REFLEX_INLINE Reflex::Int64 Reflex::Detail::QuantiseDown(Int64 value, Int64 step)
{
	value = value / step;

	return value * step;
}

REFLEX_INLINE Reflex::Float32 Reflex::Detail::QuantiseDown(Float32 value, Float32 step)
{
	value = value / step;

	value = RoundDown(value);

	return value * step;
}

REFLEX_INLINE Reflex::Float64 Reflex::Detail::QuantiseDown(Float64 value, Float64 step)
{
	value = value / step;

	value = RoundDown(value);

	return value * step;
}

REFLEX_INLINE Reflex::UInt32 Reflex::Detail::QuantiseUp(UInt32 value, UInt32 step)
{
	UInt32 q = value / step;

	q *= step;

	if (q < value) return q + step;

	return q;
}

REFLEX_INLINE Reflex::UInt64 Reflex::Detail::QuantiseUp(UInt64 value, UInt64 step)
{
	UInt64 q = value / step;

	q *= step;

	if (q < value) return q + step;

	return q;
}

REFLEX_INLINE Reflex::Int32 Reflex::Detail::QuantiseUp(Int32 value, Int32 step)
{
	Int32 q = value / step;

	q *= step;

	if (q < value) return q + step;

	return q;
}

REFLEX_INLINE Reflex::Int64 Reflex::Detail::QuantiseUp(Int64 value, Int64 step)
{
	Int64 q = value / step;

	q *= step;

	if (q < value) return q + step;

	return q;
}

REFLEX_INLINE Reflex::Float32 Reflex::Detail::QuantiseUp(Float32 value, Float32 step)
{
	value = value / step;

	value = RoundUp(value);

	return value * step;
}

REFLEX_INLINE Reflex::Float64 Reflex::Detail::QuantiseUp(Float64 value, Float64 step)
{
	value = value / step;

	value = RoundUp(value);

	return value * step;
}

REFLEX_INLINE Reflex::UInt32 Reflex::Detail::Modulo(UInt32 a, UInt32 b)
{
	return a % b;
}

REFLEX_INLINE Reflex::UInt64 Reflex::Detail::Modulo(UInt64 a, UInt64 b)
{
	return a % b;
}

REFLEX_INLINE Reflex::Int32 Reflex::Detail::Modulo(Int32 a, Int32 b)
{
	a = a % b;

	if (a < 0)
	{
		a = b + a;
	}

	return a;
}

REFLEX_INLINE Reflex::Int64 Reflex::Detail::Modulo(Int64 a, Int64 b)
{
	a = a % b;

	if (a < 0)
	{
		a = b + a;
	}

	return a;
}

REFLEX_INLINE Reflex::Float32 Reflex::Detail::Modulo(Float32 a, Float32 b)
{
	Float32 round = floor(a / b);

	a -= (round * b);

	if (a < 0.0f) a += b;

	return a;
}

REFLEX_INLINE Reflex::Float64 Reflex::Detail::Modulo(Float64 a, Float64 b)
{
	Float64 round = floor(a / b);

	a -= (round * b);

	if (a < 0.0) a += b;

	return a;
}

REFLEX_INLINE Reflex::Int32 Reflex::Detail::Abs(Int32 value)
{
	return std::abs(value);
}

REFLEX_INLINE Reflex::Int64 Reflex::Detail::Abs(Int64 value)
{
	return std::abs(value);
}

REFLEX_INLINE Reflex::Float32 Reflex::Detail::Abs(Float32 value)
{
	return std::abs(value);
}

REFLEX_INLINE Reflex::Float64 Reflex::Detail::Abs(Float64 value)
{
	return std::abs(value);
}

REFLEX_INLINE Reflex::UInt32 Reflex::Detail::SquareRoot(UInt32 value)
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

REFLEX_INLINE Reflex::Float32 Reflex::Detail::SquareRoot(Float32 value)
{
	return sqrt(value);
}

REFLEX_INLINE Reflex::Float64 Reflex::Detail::SquareRoot(Float64 value)
{
	return sqrt(value);
}

REFLEX_INLINE Reflex::Float32 Reflex::Detail::Log(Float32 x)
{
	return logf(x);
}

REFLEX_INLINE Reflex::Float64 Reflex::Detail::Log(Float64 x)
{
	return log(x);
}

REFLEX_INLINE Reflex::Float32 Reflex::Detail::Log2(Float32 x)
{
	return log2f(x);
}

REFLEX_INLINE Reflex::Float64 Reflex::Detail::Log2(Float64 x)
{
	return log2(x);
}

REFLEX_INLINE Reflex::Float32 Reflex::Detail::Exp(Float32 x)
{
	return expf(x);
}

REFLEX_INLINE Reflex::Float64 Reflex::Detail::Exp(Float64 x)
{
	return exp(x);
}

REFLEX_INLINE Reflex::Float32 Reflex::Detail::Exp2(Float32 x)
{
	return exp2f(x);
}

REFLEX_INLINE Reflex::Float64 Reflex::Detail::Exp2(Float64 x)
{
	return exp2(x);
}

REFLEX_INLINE Reflex::Float32 Reflex::Detail::Pow(Float32 x, Float32 y)
{
	return pow(x, y);
}

REFLEX_INLINE Reflex::Float64 Reflex::Detail::Pow(Float64 x, Float64 y)
{
	return pow(x, y);
}

REFLEX_INLINE Reflex::Float32 Reflex::Detail::Sin(Float32 value)
{
	return sin(value);
}

REFLEX_INLINE Reflex::Float64 Reflex::Detail::Sin(Float64 value)
{
	return sin(value);
}

REFLEX_INLINE Reflex::Float32 Reflex::Detail::Cos(Float32 value)
{
	return cos(value);
}

REFLEX_INLINE Reflex::Float64 Reflex::Detail::Cos(Float64 value)
{
	return cos(value);
}

REFLEX_INLINE Reflex::Float32 Reflex::Detail::Reciprocal(Float32 value)
{
	return 1.0f / value;
}

REFLEX_INLINE Reflex::Float64 Reflex::Detail::Reciprocal(Float64 value)
{
	return 1.0 / value;
}

REFLEX_INLINE Reflex::Float32 Reflex::Detail::LinearInterpolate(Float32 x, Float32 in0, Float32 in1)
{
	return (x * (in1 - in0)) + in0;
}

REFLEX_INLINE Reflex::Float64 Reflex::Detail::LinearInterpolate(Float64 x, Float64 in0, Float64 in1)
{
	return (x * (in1 - in0)) + in0;
}

REFLEX_INLINE Reflex::Float32 Reflex::Detail::Normalise(Float32 in, Float32 in0, Float32 in1)
{
	return (in - in0) * Reciprocal(in1 - in0);
}

REFLEX_INLINE Reflex::Float64 Reflex::Detail::Normalise(Float64 in, Float64 in0, Float64 in1)
{
	return (in - in0) * Reciprocal(in1 - in0);
}

#pragma once

#include "../core/types.h"	// Primitive platform math: usable from Core without the System object API.




//
//Secondary API

REFLEX_NS(Reflex::System)

Int32 FloatToInt32(Float32 value);

Int32 FloatToInt32(Float64 value);

Int32 Truncate(Float32 value);

Int32 Truncate(Float64 value);

Float32 RoundNearest(Float32 value);

Float64 RoundNearest(Float64 value);

Float32 RoundDown(Float32 value);

Float64 RoundDown(Float64 value);

Float32 RoundUp(Float32 value);

Float64 RoundUp(Float64 value);

Int32 Abs(Int32 value);

Int64 Abs(Int64 value);

Float32 Abs(Float32 value);

Float64 Abs(Float64 value);

Float32 MulAdd(Float32 a, Float32 b, Float32 c);

Float64 MulAdd(Float64 a, Float64 b, Float64 c);

Float32 SquareRoot(Float32 value);

Float64 SquareRoot(Float64 value);

Float32 CubeRoot(Float32 value);

Float64 CubeRoot(Float64 value);

Float32 Pow(Float32 x, Float32 y);

Float64 Pow(Float64 x, Float64 y);

Float32 Exp(Float32 x);

Float64 Exp(Float64 x);

Float32 Exp2(Float32 x);

Float64 Exp2(Float64 x);

Float32 Log(Float32 x);

Float64 Log(Float64 x);

Float32 Log2(Float32 x);

Float64 Log2(Float64 x);

Float32 Sin(Float32 value);

Float64 Sin(Float64 value);

Float32 Cos(Float32 value);

Float64 Cos(Float64 value);

Float32 Atan2(Float32 y, Float32 x);

Float64 Atan2(Float64 y, Float64 x);

REFLEX_END

#ifndef REFLEX_USE_BUILTIN_MATH
#define REFLEX_USE_BUILTIN_MATH 0
#endif

#if REFLEX_USE_BUILTIN_MATH

REFLEX_INLINE Reflex::Int32 Reflex::System::FloatToInt32(Float32 value)
{
	return Int32(__builtin_nearbyintf(value));
}

REFLEX_INLINE Reflex::Int32 Reflex::System::FloatToInt32(Float64 value)
{
	return Int32(__builtin_nearbyint(value));
}

REFLEX_INLINE Reflex::Int32 Reflex::System::Truncate(Float32 value)
{
	return Int32(value);
}

REFLEX_INLINE Reflex::Int32 Reflex::System::Truncate(Float64 value)
{
	return Int32(value);
}

REFLEX_INLINE Reflex::Float32 Reflex::System::RoundDown(Float32 value)
{
	return __builtin_floorf(value);
}

REFLEX_INLINE Reflex::Float64 Reflex::System::RoundDown(Float64 value)
{
	return __builtin_floor(value);
}

REFLEX_INLINE Reflex::Float32 Reflex::System::RoundUp(Float32 value)
{
	return __builtin_ceilf(value);
}

REFLEX_INLINE Reflex::Float64 Reflex::System::RoundUp(Float64 value)
{
	return __builtin_ceil(value);
}

REFLEX_INLINE Reflex::Int32 Reflex::System::Abs(Int32 value)
{
	return value < 0 ? -value : value;
}

REFLEX_INLINE Reflex::Int64 Reflex::System::Abs(Int64 value)
{
	return value < 0 ? -value : value;
}

REFLEX_INLINE Reflex::Float32 Reflex::System::Abs(Float32 value)
{
	return __builtin_fabsf(value);
}

REFLEX_INLINE Reflex::Float64 Reflex::System::Abs(Float64 value)
{
	return __builtin_fabs(value);
}

REFLEX_INLINE Reflex::Float32 Reflex::System::MulAdd(Float32 a, Float32 b, Float32 c)
{
	return __builtin_fmaf(a, b, c);
}

REFLEX_INLINE Reflex::Float64 Reflex::System::MulAdd(Float64 a, Float64 b, Float64 c)
{
	return __builtin_fma(a, b, c);
}

REFLEX_INLINE Reflex::Float32 Reflex::System::SquareRoot(Float32 value)
{
	return __builtin_sqrtf(value);
}

REFLEX_INLINE Reflex::Float64 Reflex::System::SquareRoot(Float64 value)
{
	return __builtin_sqrt(value);
}

#else

REFLEX_INLINE Reflex::Int32 Reflex::System::FloatToInt32(Float32 value)
{
	return Int32(std::lrint(value));
}

REFLEX_INLINE Reflex::Int32 Reflex::System::FloatToInt32(Float64 value)
{
	return Int32(std::lrint(value));
}

REFLEX_INLINE Reflex::Int32 Reflex::System::Truncate(Float32 value)
{
	return Int32(value);
}

REFLEX_INLINE Reflex::Int32 Reflex::System::Truncate(Float64 value)
{
	return Int32(value);
}

REFLEX_INLINE Reflex::Float32 Reflex::System::RoundDown(Float32 value)
{
	return std::floor(value);
}

REFLEX_INLINE Reflex::Float64 Reflex::System::RoundDown(Float64 value)
{
	return std::floor(value);
}

REFLEX_INLINE Reflex::Float32 Reflex::System::RoundUp(Float32 value)
{
	return std::ceil(value);
}

REFLEX_INLINE Reflex::Float64 Reflex::System::RoundUp(Float64 value)
{
	return std::ceil(value);
}

REFLEX_INLINE Reflex::Int32 Reflex::System::Abs(Int32 value)
{
	return std::abs(value);
}

REFLEX_INLINE Reflex::Int64 Reflex::System::Abs(Int64 value)
{
	return std::abs(value);
}

REFLEX_INLINE Reflex::Float32 Reflex::System::Abs(Float32 value)
{
	return std::abs(value);
}

REFLEX_INLINE Reflex::Float64 Reflex::System::Abs(Float64 value)
{
	return std::abs(value);
}

REFLEX_INLINE Reflex::Float32 Reflex::System::MulAdd(Float32 a, Float32 b, Float32 c)
{
	return std::fma(a, b, c);
}

REFLEX_INLINE Reflex::Float64 Reflex::System::MulAdd(Float64 a, Float64 b, Float64 c)
{
	return std::fma(a, b, c);
}

REFLEX_INLINE Reflex::Float32 Reflex::System::SquareRoot(Float32 value)
{
	return std::sqrt(value);
}

REFLEX_INLINE Reflex::Float64 Reflex::System::SquareRoot(Float64 value)
{
	return std::sqrt(value);
}

#endif

REFLEX_INLINE Reflex::Float32 Reflex::System::RoundNearest(Float32 value)
{
#ifdef REFLEX_INTEL
	return Float32(_mm_cvtss_si32(_mm_set1_ps(value)));	//last tested 2024 as 10x faster
#else
	return std::round(value);
#endif
}

REFLEX_INLINE Reflex::Float64 Reflex::System::RoundNearest(Float64 value)
{
#if defined (REFLEX_INTEL) && REFLEX_64BIT
	return Float64(_mm_cvtsd_si64(_mm_set_sd(value)));	//last tested 2024 as 10x faster
#else
	return std::round(value);
#endif
}

REFLEX_INLINE Reflex::Float32 Reflex::System::CubeRoot(Float32 value)
{
	return std::cbrt(value);
}

REFLEX_INLINE Reflex::Float64 Reflex::System::CubeRoot(Float64 value)
{
	return std::cbrt(value);
}

REFLEX_INLINE Reflex::Float32 Reflex::System::Pow(Float32 x, Float32 y)
{
	return std::pow(x, y);
}

REFLEX_INLINE Reflex::Float64 Reflex::System::Pow(Float64 x, Float64 y)
{
	return std::pow(x, y);
}

REFLEX_INLINE Reflex::Float32 Reflex::System::Exp(Float32 x)
{
	return std::exp(x);
}

REFLEX_INLINE Reflex::Float64 Reflex::System::Exp(Float64 x)
{
	return std::exp(x);
}

REFLEX_INLINE Reflex::Float32 Reflex::System::Exp2(Float32 x)
{
	return std::exp2(x);
}

REFLEX_INLINE Reflex::Float64 Reflex::System::Exp2(Float64 x)
{
	return std::exp2(x);
}

REFLEX_INLINE Reflex::Float32 Reflex::System::Log(Float32 x)
{
	return std::log(x);
}

REFLEX_INLINE Reflex::Float64 Reflex::System::Log(Float64 x)
{
	return std::log(x);
}

REFLEX_INLINE Reflex::Float32 Reflex::System::Log2(Float32 x)
{
	return std::log2(x);
}

REFLEX_INLINE Reflex::Float64 Reflex::System::Log2(Float64 x)
{
	return std::log2(x);
}

REFLEX_INLINE Reflex::Float32 Reflex::System::Sin(Float32 value)
{
	return std::sin(value);
}

REFLEX_INLINE Reflex::Float64 Reflex::System::Sin(Float64 value)
{
	return std::sin(value);
}

REFLEX_INLINE Reflex::Float32 Reflex::System::Cos(Float32 value)
{
	return std::cos(value);
}

REFLEX_INLINE Reflex::Float64 Reflex::System::Cos(Float64 value)
{
	return std::cos(value);
}

REFLEX_INLINE Reflex::Float32 Reflex::System::Atan2(Float32 y, Float32 x)
{
	return std::atan2(y, x);
}

REFLEX_INLINE Reflex::Float64 Reflex::System::Atan2(Float64 y, Float64 x)
{
	return std::atan2(y, x);
}

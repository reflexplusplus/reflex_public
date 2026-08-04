#pragma once

#include "float_v4.h"
#include "int_v4.h"




//
//Primary API

namespace Reflex::SIMD
{

	template <class TYPE> TypeV4 <TYPE> LoadUnaligned(const TYPE * unaligned);


	template <class TYPE> TypeV4 <TYPE> InterleaveLo(const TypeV4 <TYPE> & a, const TypeV4 <TYPE> & b);

	template <class TYPE> TypeV4 <TYPE> InterleaveHi(const TypeV4 <TYPE> & a, const TypeV4 <TYPE> & b);


	template <class TYPE> void Transpose(TypeV4 <TYPE> & a, TypeV4 <TYPE> & b, TypeV4 <TYPE> & c, TypeV4 <TYPE> & d);


	template <Int P0, Int P1, Int P2, Int P3, class TYPE> TypeV4 <TYPE> Shuffle(const TypeV4 <TYPE> & a, const TypeV4 <TYPE> & b);

	template <Int P0, Int P1, Int P2, Int P3, class TYPE> TypeV4 <TYPE> Shuffle(const TypeV4 <TYPE> & a);


	template <class TYPE> TypeV4 <TYPE> HiToLo(const TypeV4 <TYPE> & a, const TypeV4 <TYPE> & b);

	template <class TYPE> TypeV4 <TYPE> LoToHi(const TypeV4 <TYPE> & a, const TypeV4 <TYPE> & b);


	template <class TYPE> TYPE Sum(const TypeV4 <TYPE> & value);


	template <class TYPE> TypeV4 <TYPE> Set(const TypeV4 <TYPE> & vec, UInt vidx, TYPE value);

	template <class TYPE> TYPE Get(const TypeV4 <TYPE> & vec, UInt vidx);


	FloatV4 ToFloatV4(const IntV4 & value);


	IntV4 ToIntV4(const FloatV4 & value);

	IntV4 Truncate(const FloatV4 & value);

}




//
//impl

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4 <TYPE> Reflex::SIMD::LoadUnaligned(const TYPE * unaligned)
{
	return PlatformVectorType<TYPE,4>::LoadUnaligned(unaligned);
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4 <TYPE> Reflex::SIMD::InterleaveLo(const TypeV4 <TYPE> & a, const TypeV4 <TYPE> & b)
{
	return _mm_unpacklo_ps(a, b);
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4 <TYPE> Reflex::SIMD::InterleaveHi(const TypeV4 <TYPE> & a, const TypeV4 <TYPE> & b)
{
	return _mm_unpackhi_ps(a, b);
}

template <class TYPE> REFLEX_INLINE void Reflex::SIMD::Transpose(TypeV4 <TYPE> & a, TypeV4 <TYPE> & b, TypeV4 <TYPE> & c, TypeV4 <TYPE> & d)
{
	auto vTmp1 = REFLEX_UNPACKLO_F32_X4(a.data, b.data);
	auto vTmp2 = REFLEX_UNPACKHI_F32_X4(a.data, b.data);
	auto vTmp3 = REFLEX_UNPACKLO_F32_X4(c.data, d.data);
	auto vTmp4 = REFLEX_UNPACKHI_F32_X4(c.data, d.data);

	a = REFLEX_MOVELH_32_X4(vTmp1, vTmp3);
	b = REFLEX_MOVEHL_32_X4(vTmp3, vTmp1);
	c = REFLEX_MOVELH_32_X4(vTmp2, vTmp4);
	d = REFLEX_MOVEHL_32_X4(vTmp4, vTmp2);
}

template <Reflex::Int P0, Reflex::Int P1, Reflex::Int P2, Reflex::Int P3, class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4 <TYPE> Reflex::SIMD::Shuffle(const TypeV4 <TYPE> & a, const TypeV4 <TYPE> & b)
{
	return Reinterpret< TypeV4 <TYPE> >(REFLEX_SHUFFLE_F32_X4(Reinterpret<Float32x4>(a), Reinterpret<Float32x4>(b), _MM_SHUFFLE(P3, P2, P1, P0)));
}

template <Reflex::Int P0, Reflex::Int P1, Reflex::Int P2, Reflex::Int P3, class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4 <TYPE> Reflex::SIMD::Shuffle(const TypeV4 <TYPE> & a)
{
	return Shuffle<P0,P1,P2,P3,TYPE>(a, a);
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4 <TYPE> Reflex::SIMD::HiToLo(const TypeV4 <TYPE> & a, const TypeV4 <TYPE> & b)
{
	return Reinterpret< TypeV4 <TYPE> >(REFLEX_MOVEHL_32_X4(Reinterpret<Float32x4>(a), Reinterpret<Float32x4>(b)));
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4 <TYPE> Reflex::SIMD::LoToHi(const TypeV4 <TYPE> & a, const TypeV4 <TYPE> & b)
{
	return Reinterpret< TypeV4 <TYPE> >(REFLEX_MOVELH_32_X4(Reinterpret<Float32x4>(a), Reinterpret<Float32x4>(b)));
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::SIMD::Sum(const TypeV4 <TYPE> & value)
{
	auto t = value + HiToLo(value, value);

	t += Shuffle<1,0,2,3>(t);

	return t.ReadFirst();

	//return value[0] + value[1] + value[2] + value[3];
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4 <TYPE> Reflex::SIMD::Set(const TypeV4 <TYPE> & vec, UInt vidx, TYPE value1)
{
	auto rtn = vec;

	rtn[vidx] = value1;
	
	return rtn;
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::SIMD::Get(const TypeV4 <TYPE> & value, UInt vidx)
{
	return value[vidx];
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::ToFloatV4(const IntV4 & value)
{
	return Reinterpret<FloatV4>(REFLEX_I32_X4_TO_F32_X4(value.data));
}

REFLEX_INLINE Reflex::SIMD::IntV4 Reflex::SIMD::ToIntV4(const FloatV4 & value)
{
	return Reinterpret<IntV4>(REFLEX_F32_X4_TO_I32_X4(value.data));
}

REFLEX_INLINE Reflex::SIMD::IntV4 Reflex::SIMD::Truncate(const FloatV4 & value)
{
	return Reinterpret<IntV4>(REFLEX_TRUNCATE_F32_X4(value.data));
}

REFLEX_NS(Reflex::SIMD::Detail)

REFLEX_INLINE void AssertAlign16(const void * ptr)
{
#if REFLEX_DEBUG
	UIntNative adr = (UIntNative)ptr;

	REFLEX_ASSERT((adr & 15) == 0);
#endif
}

REFLEX_END

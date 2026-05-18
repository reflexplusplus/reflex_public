#pragma once

#include "[require].h"




//
//declarations

REFLEX_NS(Reflex::SIMD)

using Float32x4 = System::Float32x4;

using Int32x4 = System::Int32x4;

enum Boolean : UInt32
{
	kBooleanTrue = kMaxUInt32,
	kBooleanFalse = 0,
};

template <class TYPE, UInt N> struct PlatformVectorType;

template <> struct PlatformVectorType <Float32,4>
{
	using Type = Float32x4;

	REFLEX_INLINE static Type Zero() { return REFLEX_ZERO_F32_X4(); }

	REFLEX_INLINE static Type Set1(Float32 v) { return REFLEX_SET1_F32_X4(v); }
	REFLEX_INLINE static Type Set(Float32 a, Float32 b, Float32 c, Float32 d) { return REFLEX_SET_F32_X4(a, b, c, d); }

	REFLEX_INLINE static Type LoadUnaligned(const Float32 * v) { return REFLEX_LOADUNALIGNED_F32_X4(v); }
	REFLEX_INLINE static void StoreUnaligned(Float32 * dest, const Type & v4) { REFLEX_STOREUNALIGNED_F32_X4(dest, v4); }

	REFLEX_INLINE static Type Or(const Type & a, const Type & b) { return REFLEX_OR_F32_X4(a,b); }
	REFLEX_INLINE static Type And(const Type & a, const Type & b) { return REFLEX_AND_F32_X4(a,b); }
	REFLEX_INLINE static Type AndNot(const Type & a, const Type & b) { return REFLEX_ANDNOT_F32_X4(a,b); }
};

template <> struct PlatformVectorType <Int32,4>
{
	using Type = Int32x4;

	REFLEX_INLINE static Type Zero() { return REFLEX_ZERO_I32_X4(); }

	REFLEX_INLINE static Type Set1(Int32 v) { return REFLEX_SET1_I32_X4(v); }
	REFLEX_INLINE static Type Set(Int32 a, Int32 b, Int32 c, Int32 d) { return REFLEX_SET_I32_X4(a, b, c, d); }

	REFLEX_INLINE static Type LoadUnaligned(const Int32 * v) { return REFLEX_LOADUNALIGNED_I32_X4(v); }
	REFLEX_INLINE static void StoreUnaligned(Int32 * dest, const Type & v4) { REFLEX_STOREUNALIGNED_I32_X4(dest, v4); }

	REFLEX_INLINE static Type Or(const Type & a, const Type & b) { return REFLEX_OR_I32_X4(a,b); }
	REFLEX_INLINE static Type And(const Type & a, const Type & b) { return REFLEX_AND_I32_X4(a,b); }
	REFLEX_INLINE static Type AndNot(const Type & a, const Type & b) { return REFLEX_ANDNOT_I32_X4(a,b); }
};

template <> struct PlatformVectorType <Boolean,4> : public PlatformVectorType <Int32,4>
{
	REFLEX_INLINE static Type Set1(Boolean v) { return REFLEX_SET1_I32_X4(v); }
	REFLEX_INLINE static Type Set(Boolean a, Boolean b, Boolean c, Boolean d) { return REFLEX_SET_I32_X4(a, b, c, d); }

	REFLEX_INLINE static void StoreUnaligned(Boolean * dest, const Type & v4) { REFLEX_STOREUNALIGNED_I32_X4(Reinterpret<Int32>(dest), v4); }
};

REFLEX_END

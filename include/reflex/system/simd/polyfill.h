#pragma once

#include "../[require].h"




//
//

namespace Reflex::System
{
	template <class TYPE> struct alignas(16) SIMDx4
	{
		TYPE a, b, c, d;
	};

	template <class TYPE> struct alignas(16) SIMDx2
	{
		TYPE a, b;
	};

	using Float32x4 = SIMDx4 <Float32>;
	using Int32x4 = SIMDx4 <Int32>;
	using Float64x2 = SIMDx2 <Float64>;

	inline Int32x4 Int32x4_Or(const Int32x4 & a, const Int32x4 & b) { return { a.a | b.a, a.b | b.b, a.c | b.c, a.d | b.d }; }
	inline Int32x4 Int32x4_Xor(const Int32x4 & a, const Int32x4 & b) { return { a.a ^ b.a, a.b ^ b.b, a.c ^ b.c, a.d ^ b.d  }; }
	inline Int32x4 Int32x4_And(const Int32x4 & a, const Int32x4 & b) { return { a.a & b.a, a.b & b.b, a.c & b.c, a.d & b.d }; }
	inline Int32x4 Int32x4_AndNot(const Int32x4 & a, const Int32x4 & b) { return { (~a.a) & b.a, (~a.b) & b.b, (~a.c) & b.c, (~a.d) & b.d }; }

	inline int MoveMask(const Int32x4 & a)
	{
		constexpr auto signbit_polyfill = [](UInt32 u) -> bool
		{
			return (u >> 31) & 1;
		};

		int mask = 0;

		auto ptr = &a.a;

		// Check the sign bit of each of the four floats in the __m128
		for (int i = 0; i < 4; i++)
		{
			if (signbit_polyfill(ptr[i]))
			{
				mask |= (1 << i); // Set the i-th bit of the mask
			}
		}

		return mask;
	}

	template <class TYPE> struct OpEQ { static bool Invoke(TYPE a, TYPE b) { return a == b; } };
	template <class TYPE> struct OpIEQ { static bool Invoke(TYPE a, TYPE b) { return a != b; } };
	template <class TYPE> struct OpGT { static bool Invoke(TYPE a, TYPE b) { return a > b; } };
	template <class TYPE> struct OpGTE { static bool Invoke(TYPE a, TYPE b) { return a >= b; } };
	template <class TYPE> struct OpLT { static bool Invoke(TYPE a, TYPE b) { return a < b; } };
	template <class TYPE> struct OpLTE { static bool Invoke(TYPE a, TYPE b) { return a <= b; } };

	constexpr Int32 kBooleanTrue = 0xFFFFFFFF;

	template <class TYPE, class OP> inline Int32x4 V4_LogicOp(const SIMDx4 <TYPE> & a, const SIMDx4 <TYPE> & b) { return { OP::Invoke(a.a, b.a) ? kBooleanTrue : 0, OP::Invoke(a.b, b.b) ? kBooleanTrue : 0, OP::Invoke(a.c, b.c) ? kBooleanTrue : 0, OP::Invoke(a.d, b.d) ? kBooleanTrue : 0 }; }

	template <class TYPE> inline SIMDx4 <TYPE> V4_Min(const SIMDx4 <TYPE> & a, const SIMDx4 <TYPE> & b) { return { Min(a.a, b.a), Min(a.b, b.b), Min(a.c, b.c), Min(a.d, b.d) }; }
	template <class TYPE> inline SIMDx4 <TYPE> V4_Max(const SIMDx4 <TYPE> & a, const SIMDx4 <TYPE> & b) { return { Max(a.a, b.a), Max(a.b, b.b), Max(a.c, b.c), Max(a.d, b.d) }; }

	template <class TYPE> inline SIMDx4 <TYPE> V4_Add(const SIMDx4 <TYPE> & a, const SIMDx4 <TYPE> & b) { return { a.a + b.a, a.b + b.b, a.c + b.c, a.d + b.d }; }
	template <class TYPE> inline SIMDx4 <TYPE> V4_Sub(const SIMDx4 <TYPE> & a, const SIMDx4 <TYPE> & b) { return { a.a - b.a, a.b - b.b, a.c - b.c, a.d - b.d }; }
	template <class TYPE> inline SIMDx4 <TYPE> V4_Mul(const SIMDx4 <TYPE> & a, const SIMDx4 <TYPE> & b) { return { a.a * b.a, a.b * b.b, a.c * b.c, a.d * b.d }; }
	template <class TYPE> inline SIMDx4 <TYPE> V4_Div(const SIMDx4 <TYPE> & a, const SIMDx4 <TYPE> & b) { return { a.a / b.a, a.b / b.b, a.c / b.c, a.d / b.d }; }

	inline Float32x4 V4_Rcp(const Float32x4 & V4) { return { 1.0f / V4.a, 1.0f / V4.b, 1.0f / V4.c, 1.0f / V4.d }; }
	inline Float32x4 V4_Sqrt(const Float32x4 & V4) { return { Reflex::System::SquareRoot(V4.a), Reflex::System::SquareRoot(V4.b), Reflex::System::SquareRoot(V4.c), Reflex::System::SquareRoot(V4.d) }; }

	template <unsigned WORD> inline Float32x4 V4_Shuffle(const Float32x4 & a, const Float32x4 & b)
	{
		const Float32 first[] = { a.a, a.b, a.c, a.d };
		const Float32 second[] = { b.a, b.b, b.c, b.d };
		return { first[WORD & 3], first[(WORD >> 2) & 3], second[(WORD >> 4) & 3], second[(WORD >> 6) & 3] };
	}
	inline Float32x4 V4_UnpackLow(const Float32x4 & a, const Float32x4 & b) { return { a.a, b.a, a.b, b.b }; }
	inline Float32x4 V4_UnpackHigh(const Float32x4 & a, const Float32x4 & b) { return { a.c, b.c, a.d, b.d }; }

	inline Int32x4 V4_SHL(const Int32x4 & A, Int32 shift) { return { A.a << shift, A.b << shift, A.c << shift, A.d << shift }; }
	inline Int32x4 V4_SHR(const Int32x4 & A, Int32 shift) { return { Int32(UInt32(A.a) >> shift), Int32(UInt32(A.b) >> shift), Int32(UInt32(A.c) >> shift), Int32(UInt32(A.d) >> shift) }; }


	template <class TYPE> inline SIMDx2 <TYPE> V2_Add(const SIMDx2 <TYPE> & a, const SIMDx2 <TYPE> & b) { return { a.a + b.a, a.b + b.b }; }
	template <class TYPE> inline SIMDx2 <TYPE> V2_Sub(const SIMDx2 <TYPE> & a, const SIMDx2 <TYPE> & b) { return { a.a - b.a, a.b - b.b }; }
	template <class TYPE> inline SIMDx2 <TYPE> V2_Mul(const SIMDx2 <TYPE> & a, const SIMDx2 <TYPE> & b) { return { a.a * b.a, a.b * b.b }; }
	template <class TYPE> inline SIMDx2 <TYPE> V2_Div(const SIMDx2 <TYPE> & a, const SIMDx2 <TYPE> & b) { return { a.a / b.a, a.b / b.b }; }


	template <class TYPE> inline SIMDx4 <TYPE> SET1_X4(TYPE value) { return { value, value, value, value }; }

	inline Float64x2 F32_X4_TO_F64_X2(const Float32x4 & value) { return { Float64(value.a), Float64(value.b) }; }
	inline Float32x4 F64_X2_TO_F32_X4(const Float64x2 & value) { return { Float32(value.a), Float32(value.b), 0.0f, 0.0f }; }
}

#define REFLEX_MOVEHL_32_X4(A, B) (Reflex::System::Float32x4{ B.c, B.d, A.c, A.d })
#define REFLEX_MOVELH_32_X4(A, B) (Reflex::System::Float32x4{ A.a, A.b, B.a, B.b })
#define REFLEX_SHUFFLE_F32_X4(A, B, word) Reflex::System::V4_Shuffle<word>(A, B)
#define REFLEX_UNPACKLO_F32_X4(A, B) Reflex::System::V4_UnpackLow(A, B)
#define REFLEX_UNPACKHI_F32_X4(A, B) Reflex::System::V4_UnpackHigh(A, B)
#define REFLEX_GETFIRST_F32_X4(A) (A.a)

#define REFLEX_LOADUNALIGNED_F32_X4(ptr) (Reflex::System::Float32x4{ ptr[0], ptr[1], ptr[2], ptr[3] })
#define REFLEX_STOREUNALIGNED_F32_X4(ptr, v4) { ptr[0] = v4.a; ptr[1] = v4.b; ptr[2] = v4.c; ptr[3] = v4.d; }

#define REFLEX_ZERO_F32_X4() Reflex::System::SET1_X4<Reflex::Float32>(0.0f)
#define REFLEX_SET1_F32_X4(value) Reflex::System::SET1_X4<Reflex::Float32>(value)
// Match the native set intrinsics: arguments run from highest lane to lowest.
#define REFLEX_SET_F32_X4(a, b, c, d) (Reflex::System::Float32x4{ Reflex::Float32(d), Reflex::Float32(c), Reflex::Float32(b), Reflex::Float32(a) })

#define REFLEX_OR_F32_X4(A,B) REFLEX_SIMD_CAST(Float32x4, REFLEX_OR_I32_X4(REFLEX_SIMD_CAST(Int32x4,A),REFLEX_SIMD_CAST(Int32x4,B)))
#define REFLEX_XOR_F32_X4(a,b) REFLEX_SIMD_CAST(Float32x4, REFLEX_XOR_I32_X4(REFLEX_SIMD_CAST(Int32x4,a),REFLEX_SIMD_CAST(Int32x4,b)))
#define REFLEX_AND_F32_X4(a,b) REFLEX_SIMD_CAST(Float32x4, REFLEX_AND_I32_X4(REFLEX_SIMD_CAST(Int32x4,a),REFLEX_SIMD_CAST(Int32x4,b)))
#define REFLEX_ANDNOT_F32_X4(a, b) REFLEX_SIMD_CAST(Float32x4,REFLEX_ANDNOT_I32_X4(REFLEX_SIMD_CAST(Int32x4,a),REFLEX_SIMD_CAST(Int32x4,b)))

#define REFLEX_EQ_F32_X4(a,b) Reflex::System::V4_LogicOp<float,Reflex::System::OpEQ<float>>(a,b)
#define REFLEX_IEQ_F32_X4(a,b) Reflex::System::V4_LogicOp<float,Reflex::System::OpIEQ<float>>(a,b)
#define REFLEX_GT_F32_X4(a,b) Reflex::System::V4_LogicOp<float,Reflex::System::OpGT<float>>(a,b)
#define REFLEX_GTEQ_F32_X4(a,b) Reflex::System::V4_LogicOp<float,Reflex::System::OpGTE<float>>(a,b)
#define REFLEX_LT_F32_X4(a,b) Reflex::System::V4_LogicOp<float,Reflex::System::OpLT<float>>(a,b)
#define REFLEX_LTEQ_F32_X4(a,b) Reflex::System::V4_LogicOp<float,Reflex::System::OpLTE<float>>(a,b)

#define REFLEX_MIN_F32_X4(A, B) Reflex::System::V4_Min<float>(A,B)
#define REFLEX_MAX_F32_X4(A, B) Reflex::System::V4_Max<float>(A,B)

#define REFLEX_ADD_F32_X4(A, B) Reflex::System::V4_Add<Float32>(A,B)
#define REFLEX_SUB_F32_X4(A, B) Reflex::System::V4_Sub<Float32>(A,B)
#define REFLEX_MUL_F32_X4(A, B) Reflex::System::V4_Mul<Float32>(A,B)
#define REFLEX_MULADD_F32_X4(A, B, C) REFLEX_ADD_F32_X4(REFLEX_MUL_F32_X4(A, B), C)
#define REFLEX_DIV_F32_X4(A, B) Reflex::System::V4_Div<Float32>(A,B)
#define REFLEX_RCP_F32_X4(V4) Reflex::System::V4_Rcp(V4)
#define REFLEX_SQRT_F32_X4(V4) Reflex::System::V4_Sqrt(V4)


#define REFLEX_SET_F64_X2(A, B) (Reflex::System::Float64x2{ Float64(B), Float64(A) })
#define REFLEX_ADD_F64_X2(A, B) Reflex::System::V2_Add<Float64>(A,B)
#define REFLEX_SUB_F64_X2(A, B) Reflex::System::V2_Sub<Float64>(A,B)
#define REFLEX_MUL_F64_X2(A, B) Reflex::System::V2_Mul<Float64>(A,B)
#define REFLEX_DIV_F64_X2(A, B) Reflex::System::V2_Div<Float64>(A,B)
#define REFLEX_F32_X4_TO_F64_X2(value) Reflex::System::F32_X4_TO_F64_X2(value)
#define REFLEX_F64_X2_TO_F32_X4(value) Reflex::System::F64_X2_TO_F32_X4(value)


#define REFLEX_LOADUNALIGNED_I32_X4(ptr) (Reflex::System::Int32x4{ ptr[0],ptr[1],ptr[2],ptr[3] })
#define REFLEX_STOREUNALIGNED_I32_X4(ptr, v4) { ptr[0] = v4.a; ptr[1] = v4.b; ptr[2] = v4.c; ptr[3] = v4.d; }

#define REFLEX_ZERO_I32_X4() Reflex::System::SET1_X4<Reflex::Int32>(0)
#define REFLEX_SET1_I32_X4(value) Reflex::System::SET1_X4<Reflex::Int32>(value)
#define REFLEX_SET_I32_X4(a, b, c, d) (Reflex::System::Int32x4{ Reflex::Int32(d), Reflex::Int32(c), Reflex::Int32(b), Reflex::Int32(a) })

#define REFLEX_SHL1_I32_X4(A,shift) Reflex::System::V4_SHL(A, shift)
#define REFLEX_SHR1_I32_X4(A,shift) Reflex::System::V4_SHR(A, shift)

#define REFLEX_OR_I32_X4(a,b) Reflex::System::Int32x4_Or(a,b)
#define REFLEX_XOR_I32_X4(a,b) Reflex::System::Int32x4_Xor(a,b)
#define REFLEX_AND_I32_X4(a,b) Reflex::System::Int32x4_And(a,b)
#define REFLEX_ANDNOT_I32_X4(a,b) Reflex::System::Int32x4_AndNot(a,b)

#define REFLEX_EQ_I32_X4(a,b) Reflex::System::V4_LogicOp<int,Reflex::System::OpEQ<int>>(a,b)
#define REFLEX_IEQ_I32_X4(a,b) Reflex::System::V4_LogicOp<int,Reflex::System::OpIEQ<int>>(a,b)
#define REFLEX_GT_I32_X4(a,b) Reflex::System::V4_LogicOp<int,Reflex::System::OpGT<int>>(a,b)
#define REFLEX_GTEQ_I32_X4(a,b) Reflex::System::V4_LogicOp<int,Reflex::System::OpGTE<int>>(a,b)
#define REFLEX_LT_I32_X4(a,b) Reflex::System::V4_LogicOp<int,Reflex::System::OpLT<int>>(a,b)
#define REFLEX_LTEQ_I32_X4(a,b) Reflex::System::V4_LogicOp<int,Reflex::System::OpLTE<int>>(a,b)

#define REFLEX_ADD_I32_X4(A, B) Reflex::System::V4_Add<int>(A,B)
#define REFLEX_SUB_I32_X4(A, B) Reflex::System::V4_Sub<int>(A,B)
#define REFLEX_MUL_I32_X4(A, B) Reflex::System::V4_Mul<int>(A,B)

#define REFLEX_F32_X4_TO_I32_X4(V4) (Reflex::System::Int32x4{ Reflex::ToInt32(V4.a),Reflex::ToInt32(V4.b),Reflex::ToInt32(V4.c),Reflex::ToInt32(V4.d) })
#define REFLEX_TRUNCATE_F32_X4(V4) (Reflex::System::Int32x4{ Reflex::Truncate(V4.a),Reflex::Truncate(V4.b),Reflex::Truncate(V4.c),Reflex::Truncate(V4.d) })

#define REFLEX_I32_X4_TO_F32_X4(V4) (Reflex::System::Float32x4{ float(V4.a),float(V4.b),float(V4.c),float(V4.d) })

#define REFLEX_I32_X4_TO_I32_MASK(i32_v4) Reflex::System::MoveMask(i32_v4)




//
//impl

#define REFLEX_SIMD_CAST(T,V) Reflex::Reinterpret<Reflex::System::T>(V)

inline void REFLEX_ENABLE_FTZ(bool enable = true)
{

}

#pragma once

#include "../preprocessor/intel.h"




//
//

namespace Reflex::System
{

	using Float32x4 = __m128;
	using Int32x4 = __m128i;
	using Float64x2 = __m128d;

}

#define REFLEX_MOVEHL_32_X4(a,b) _mm_movehl_ps(a, b)
#define REFLEX_MOVELH_32_X4(a,b) _mm_movelh_ps(a, b)
#define REFLEX_SHUFFLE_F32_X4(a,b,word) _mm_shuffle_ps(a,b,word)

#define REFLEX_UNPACKLO_F32_X4(a,b) _mm_unpacklo_ps(a, b)
#define REFLEX_UNPACKHI_F32_X4(a,b) _mm_unpackhi_ps(a, b)

#define REFLEX_GETFIRST_F32_X4(a) _mm_cvtss_f32(a)
//#define REFLEX_GET_F32_X4(a, elem) _mm_extract_ps(a, elem)

#define REFLEX_LOADUNALIGNED_F32_X4(f32ptr) _mm_loadu_ps(f32ptr)
#define REFLEX_STOREUNALIGNED_F32_X4(f32ptr, v4) _mm_storeu_ps(f32ptr, v4)

#define REFLEX_ZERO_F32_X4() _mm_setzero_ps()
#define REFLEX_SET1_F32_X4(f32) _mm_set1_ps(f32)
#define REFLEX_SET_F32_X4(a, b, c, d) _mm_set_ps(a, b, c, d)

#define REFLEX_OR_F32_X4(a,b) _mm_or_ps(a,b)
#define REFLEX_XOR_F32_X4(a,b) _mm_xor_ps(a,b)
#define REFLEX_AND_F32_X4(a,b) _mm_and_ps(a,b)
#define REFLEX_ANDNOT_F32_X4(a,b) _mm_andnot_ps(a,b)

#define REFLEX_EQ_F32_X4(a,b) REFLEX_F32_X4_LOGIC_OP(_mm_cmpeq_ps,a,b)
#define REFLEX_IEQ_F32_X4(a,b) REFLEX_F32_X4_LOGIC_OP(_mm_cmpneq_ps,a,b)
#define REFLEX_GT_F32_X4(a,b) REFLEX_F32_X4_LOGIC_OP(_mm_cmpgt_ps,a,b)
#define REFLEX_GTEQ_F32_X4(a,b) REFLEX_F32_X4_LOGIC_OP(_mm_cmpge_ps,a,b)
#define REFLEX_LT_F32_X4(a,b) REFLEX_F32_X4_LOGIC_OP(_mm_cmplt_ps,a,b)
#define REFLEX_LTEQ_F32_X4(a,b) REFLEX_F32_X4_LOGIC_OP(_mm_cmple_ps,a,b)

#define REFLEX_MIN_F32_X4(a,b) _mm_min_ps(a,b)
#define REFLEX_MAX_F32_X4(a,b) _mm_max_ps(a,b)

#define REFLEX_ADD_F32_X4(a,b) _mm_add_ps(a,b)
#define REFLEX_SUB_F32_X4(a,b) _mm_sub_ps(a,b)
#define REFLEX_MUL_F32_X4(a,b) _mm_mul_ps(a,b)
#define REFLEX_DIV_F32_X4(a,b) _mm_div_ps(a,b)

#ifdef REFLEX_OS_WINDOWS
#define REFLEX_MULADD_F32_X4(a,b,c) _mm_fmadd_ps((a),(b),(c))
#else
#define REFLEX_MULADD_F32_X4(a,b,c) _mm_add_ps(_mm_mul_ps((a),(b)), (c))
#endif

#define REFLEX_RCP_F32_X4(v) _mm_rcp_ps(v)
#define REFLEX_SQRT_F32_X4(v) _mm_sqrt_ps(v)


#define REFLEX_SET_F64_X2(v1, v0) _mm_set_pd(v1, v0)

#define REFLEX_ADD_F64_X2(a,b) _mm_add_pd(a,b)
#define REFLEX_SUB_F64_X2(a,b) _mm_sub_pd(a,b)
#define REFLEX_MUL_F64_X2(a,b) _mm_mul_pd(a,b)
#define REFLEX_DIV_F64_X2(a,b) _mm_div_pd(a,b)

#define REFLEX_F32_X4_TO_F64_X2(a) _mm_cvtps_pd(a)
#define REFLEX_F64_X2_TO_F32_X4(a) _mm_cvtpd_ps(a)


#define REFLEX_SHUFFLE_I32_X4(a,b,word) _mm_castps_si128(REFLEX_SHUFFLE_F32_X4(_mm_castsi128_ps(a),_mm_castsi128_ps(b),word))

#define REFLEX_LOADUNALIGNED_I32_X4(i32ptr) _mm_lddqu_si128(reinterpret_cast<const Reflex::System::Int32x4*>(i32ptr))
#define REFLEX_STOREUNALIGNED_I32_X4(i32ptr, v4) _mm_storeu_si128(reinterpret_cast<Reflex::System::Int32x4*>(i32ptr), v4)

#define REFLEX_ZERO_I32_X4() _mm_setzero_si128()
#define REFLEX_SET1_I32_X4(i32) _mm_set1_epi32(i32)
#define REFLEX_SET_I32_X4(a, b, c, d) _mm_set_epi32(a, b, c, d)

#define REFLEX_SHL1_I32_X4(a4,b1) _mm_slli_epi32(a4,b1)
#define REFLEX_SHR1_I32_X4(a4,b1) _mm_srli_epi32(a4,b1)

#define REFLEX_OR_I32_X4(a,b) _mm_or_si128(a,b)
#define REFLEX_XOR_I32_X4(a,b) _mm_xor_si128(a,b)
#define REFLEX_AND_I32_X4(a,b) _mm_and_si128(a,b)
#define REFLEX_ANDNOT_I32_X4(a,b) _mm_andnot_si128(a,b)

#define REFLEX_EQ_I32_X4(a,b) _mm_cmpeq_epi32(a,b)
#define REFLEX_IEQ_I32_X4(a,b) todo
#define REFLEX_GT_I32_X4(a,b) _mm_cmpgt_epi32(a,b)
#define REFLEX_GTEQ_I32_X4(a,b) todo
#define REFLEX_LT_I32_X4(a,b) _mm_cmplt_epi32(a,b)
#define REFLEX_LTEQ_I32_X4(a,b) todo

#define REFLEX_ADD_I32_X4(a,b) _mm_add_epi32(a,b)
#define REFLEX_SUB_I32_X4(a,b) _mm_sub_epi32(a,b)
#define REFLEX_MUL_I32_X4(a,b) _mm_mul_epi32(a,b)

#define REFLEX_F32_X4_TO_I32_X4(f32_v4) _mm_cvtps_epi32(f32_v4)	//round nearest
#define REFLEX_TRUNCATE_F32_X4(f32_v4) _mm_cvttps_epi32(f32_v4)	//round down

#define REFLEX_I32_X4_TO_F32_X4(i32_v4) _mm_cvtepi32_ps(i32_v4)

#define REFLEX_I32_X4_TO_I32_MASK(i32_v4) _mm_movemask_ps(reinterpret_cast<const Reflex::System::Float32x4&>(i32_v4))

void REFLEX_ENABLE_FTZ();




//
//impl

#define REFLEX_F32_X4_LOGIC_OP(OP,a,b) OP(a,b)

inline void REFLEX_ENABLE_FTZ()
{
	const unsigned flags = (1u << 6) | (1u << 15); // DAZ|FTZ
	
	unsigned mx = _mm_getcsr();
	
	if ((mx & flags) != flags) _mm_setcsr(mx | flags);

	//if (enable)
	//{
	//	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);		//_mm_setcsr ((_mm_getcsr () & ~0x0040) | (0x0040));

	//	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	//}
	//else
	//{
	//	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_OFF);	//_mm_setcsr ((_mm_getcsr () & ~0x0040) | (0x0040));	//replaces above

	//	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_OFF);
	//}
}

#pragma once

#include "../preprocessor/arm.h"




//
//defines

namespace Reflex::System
{

	using Float32x4 = float32x4_t;
	using Int32x4 = int32x4_t;
	using Float64x2 = float64x2_t;

}

#define REFLEX_MOVEHL_32_X4(a,b) vcombine_f32(vget_high_f32(b), vget_high_f32(a))
#define REFLEX_MOVELH_32_X4(a,b) vcombine_f32(vget_low_f32(a), vget_low_f32(b))

#define REFLEX_SHUFFLE_F32_X4(a, b, imm) __builtin_shufflevector(a, b, (imm) & (0x3), ((imm) >> 2) & 0x3, (((imm) >> 4) & 0x3) + 4, (((imm) >> 6) & 0x3) + 4)

#define REFLEX_UNPACKLO_F32_X4(a,b) vzip1q_f32(a,b)
#define REFLEX_UNPACKHI_F32_X4(a,b) vzip2q_f32(a,b)

#define REFLEX_GETFIRST_F32_X4(a) vgetq_lane_f32(a, 0)
//#define REFLEX_GET_F32_X4(a, elem) vgetq_lane_f32(a, elem)

#define REFLEX_LOADUNALIGNED_F32_X4(f32ptr) vld1q_f32(f32ptr)
#define REFLEX_STOREUNALIGNED_F32_X4(f32ptr, v4) vst1q_f32(f32ptr, v4)

#define REFLEX_ZERO_F32_X4() vdupq_n_f32(0)
#define REFLEX_SET1_F32_X4(f32) vdupq_n_f32(f32)
float32x4_t REFLEX_SET_F32_X4(float v3, float v2, float v1, float v0);

#define REFLEX_OR_F32_X4(a, b) REFLEX_F32_X4_LOGIC_OP(vorrq_s32, a, b)
#define REFLEX_XOR_F32_X4(a, b) REFLEX_F32_X4_LOGIC_OP(veorq_s32, a, b)
#define REFLEX_AND_F32_X4(a, b) REFLEX_F32_X4_LOGIC_OP(vandq_s32, a, b)
#define REFLEX_ANDNOT_F32_X4(a, b) REFLEX_F32_X4_LOGIC_OP(vbicq_s32, b, a)

#define REFLEX_EQ_F32_X4(a,b) vreinterpretq_s32_u32(vceqq_f32(a,b))
#define REFLEX_IEQ_F32_X4(a,b) vreinterpretq_s32_u32(vmvnq_u32(vceqq_f32(a, b)))
#define REFLEX_GT_F32_X4(a,b) vreinterpretq_s32_u32(vcgtq_f32(a,b))
#define REFLEX_GTEQ_F32_X4(a,b) vreinterpretq_s32_u32(vcgeq_f32(a,b))
#define REFLEX_LT_F32_X4(a,b) vreinterpretq_s32_u32(vcltq_f32(a,b))
#define REFLEX_LTEQ_F32_X4(a,b) vreinterpretq_s32_u32(vcleq_f32(a,b))

#define REFLEX_MIN_F32_X4(a,b) vminq_f32(a,b)
#define REFLEX_MAX_F32_X4(a,b) vmaxq_f32(a,b)

#define REFLEX_ADD_F32_X4(a,b) vaddq_f32(a,b)
#define REFLEX_SUB_F32_X4(a,b) vsubq_f32(a,b)
#define REFLEX_MUL_F32_X4(a,b) vmulq_f32(a,b)
#define REFLEX_DIV_F32_X4(a,b) vdivq_f32(a,b)
#define REFLEX_MULADD_F32_X4(a,b,c) vfmaq_f32((c),(a),(b))

float32x4_t REFLEX_RCP_F32_X4(float32x4_t v);
#define REFLEX_SQRT_F32_X4(v) vsqrtq_f32(v)


#define REFLEX_ZERO_F64_X2() vdupq_n_f64(0.0)
#define REFLEX_SET1_F64_X2(f64) vdupq_n_f64(f64)
float64x2_t REFLEX_SET_F64_X2(double v1, double v0);

#define REFLEX_ADD_F64_X2(a,b) vaddq_f64(a,b)
#define REFLEX_SUB_F64_X2(a,b) vsubq_f64(a,b)
#define REFLEX_MUL_F64_X2(a,b) vmulq_f64(a,b)
#define REFLEX_DIV_F64_X2(a,b) vdivq_f64(a,b)

#define REFLEX_F32_X4_TO_F64_X2(a) vcvt_f64_f32(vget_low_f32(a))
#define REFLEX_F64_X2_TO_F32_X4(a) vcombine_f32(vcvt_f32_f64(a), vdup_n_f32(0))


#define REFLEX_SHUFFLE_I32_X4(a, b, imm) vreinterpretq_s32_f32(REFLEX_SHUFFLE_F32_X4(vreinterpretq_f32_s32(a), vreinterpretq_f32_s32(b), imm))

#define REFLEX_LOADUNALIGNED_I32_X4(f32ptr) vld1q_s32(f32ptr)
#define REFLEX_STOREUNALIGNED_I32_X4(f32ptr, v4) vst1q_s32(f32ptr, v4)

#define REFLEX_ZERO_I32_X4() vdupq_n_s32(0)
#define REFLEX_SET1_I32_X4(i32) vdupq_n_s32(i32)
int32x4_t REFLEX_SET_I32_X4(int i3, int i2, int i1, int i0);

int32x4_t REFLEX_SHL1_I32_X4(int32x4_t i32_v4, int shift);
int32x4_t REFLEX_SHR1_I32_X4(int32x4_t i32_v4, int shift);
#define REFLEX_OR_I32_X4(a,b) vorrq_s32(a,b)
#define REFLEX_XOR_I32_X4(a,b) veorq_s32(a,b)
#define REFLEX_AND_I32_X4(a,b) vandq_s32(a,b)
#define REFLEX_ANDNOT_I32_X4(a,b) vbicq_s32(b,a)

#define REFLEX_EQ_I32_X4(a,b) vceqq_s32(a, b)
#define REFLEX_IEQ_I32_X4(a,b) todo
#define REFLEX_GT_I32_X4(a,b) vcgtq_s32(a,b)
#define REFLEX_GTEQ_I32_X4(a,b) vcgeq_s32(a,b)
#define REFLEX_LT_I32_X4(a,b) vcltq_s32(a,b)
#define REFLEX_LTEQ_I32_X4(a,b) vcleq_s32(a,b)

#define REFLEX_ADD_I32_X4(a,b) vaddq_s32(a,b)
#define REFLEX_SUB_I32_X4(a,b) vsubq_s32(a,b)
#define REFLEX_MUL_I32_X4(a,b) vmulq_s32(a,b)

#define REFLEX_F32_X4_TO_I32_X4(f32_v4) vcvtnq_s32_f32(f32_v4)	//round nearest

#define REFLEX_TRUNCATE_F32_X4(f32_v4) vcvtq_s32_f32(f32_v4)	//round down

#define REFLEX_I32_X4_TO_F32_X4(i32_v4) vcvtq_f32_s32(i32_v4)

int REFLEX_I32_X4_TO_I32_MASK(int32x4_t i32_v4);




inline void REFLEX_ENABLE_FTZ()
{
	const unsigned long long FZ = 1ull << 24;

	unsigned long long fpcr =
#if __has_builtin(__builtin_arm_get_fpcr)
	__builtin_arm_get_fpcr();
#else
	({ unsigned long long r; asm volatile("mrs %0, fpcr":"=r"(r)); r; });
#endif
	if ((fpcr & FZ) == 0)
	{
#if __has_builtin(__builtin_arm_set_fpcr)
	__builtin_arm_set_fpcr(fpcr | FZ);
#else
	asm volatile("msr fpcr, %0"::"r"(fpcr | FZ));
#endif
	}
}

//inline void REFLEX_ENABLE_FTZ(bool enable)
//{
//	uint64_t ftzbitmask = 0;
//
//	ftzbitmask = ftzbitmask | (uint64_t(1) << uint64_t(24));	//FZ
//	ftzbitmask = ftzbitmask | (uint64_t(1) << uint64_t(1));		//AH  (denormal outputs)
//	ftzbitmask = ftzbitmask | (uint64_t(1) << uint64_t(0));		//FIZ (denormal inputs)
//
//	uint64_t r;
//
//	asm volatile("mrs %0, FPCR" : "=r"(r)); /* read */
//
//	if (enable)
//	{
//		r = r | ftzbitmask;
//	}
//	else
//	{
//		r = r & ~ftzbitmask;
//	}
//
//	asm volatile("msr FPCR, %0" ::"r"(r)); /* write */
//}




//
//IMPL

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define REFLEX_ALIGN_STRUCT(x) __attribute__((aligned(x)))

#define REFLEX_F32_X4_LOGIC_OP(OP,a,b) vreinterpretq_f32_s32(OP(vreinterpretq_s32_f32(a), vreinterpretq_s32_f32(b)))

REFLEX_INLINE float32x4_t REFLEX_SET_F32_X4(float w, float z, float y, float x)
{
	float REFLEX_ALIGN_STRUCT(16) data[4] = {x, y, z, w};

	return vld1q_f32(data);
}

REFLEX_INLINE float64x2_t REFLEX_SET_F64_X2(double v1, double v0)
{
	double REFLEX_ALIGN_STRUCT(16) data[4] = {v0, v1};

	return vld1q_f64(data);
}

REFLEX_INLINE int32x4_t REFLEX_SET_I32_X4(int i3, int i2, int i1, int i0)
{
	int32_t REFLEX_ALIGN_STRUCT(16) data[4] = {i0, i1, i2, i3};

	return vld1q_s32(data);
}

REFLEX_INLINE float32x4_t REFLEX_RCP_F32_X4(float32x4_t a)
{
	float32x4_t rcp_approx = vrecpeq_f32(a);

	return vmulq_f32(rcp_approx, vrecpsq_f32(rcp_approx, a));
}

REFLEX_INLINE int32x4_t REFLEX_SHL1_I32_X4(int32x4_t a, int imm)
{
	if (unlikely(imm <= 0)) return a;

	if (unlikely(imm > 31)) return vdupq_n_s32(0);

	return vshlq_s32(a, vdupq_n_s32(imm));
}

REFLEX_INLINE int32x4_t REFLEX_SHR1_I32_X4(int32x4_t a, int imm)
{
	if (unlikely(imm <= 0)) return a;

	if (likely(imm < 32)) return vreinterpretq_s32_u32(vshlq_u32(vreinterpretq_u32_s32(a), vdupq_n_s32(-imm)));

	return vdupq_n_s32(0);
}

inline const int32x4_t kREFLEX_I32_X4_TO_I32_MASK_shift = {0, 1, 2, 3};

REFLEX_INLINE int REFLEX_I32_X4_TO_I32_MASK(int32x4_t v)
{
	return vaddvq_u32(vshlq_u32(vshrq_n_u32(vreinterpretq_u32_s32(v), 31), kREFLEX_I32_X4_TO_I32_MASK_shift));
}

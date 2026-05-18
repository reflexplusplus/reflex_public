#pragma once

#include "bool_v4.h"




//
//Primary API

namespace Reflex::SIMD
{

	using FloatV4 = TypeV4 <Float32>;


	BoolV4 operator==(const FloatV4 & a, const FloatV4 & b);

	BoolV4 operator!=(const FloatV4 & a, const FloatV4 & b);

	BoolV4 operator>(const FloatV4 & a, const FloatV4 & b);

	BoolV4 operator>=(const FloatV4 & a, const FloatV4 & b);

	BoolV4 operator<(const FloatV4 & a, const FloatV4 & b);

	BoolV4 operator<=(const FloatV4 & a, const FloatV4 & b);


	FloatV4 operator-(const FloatV4 & value);

	FloatV4 operator+(const FloatV4 & a, const FloatV4 & b);

	FloatV4 operator-(const FloatV4 & a, const FloatV4 & b);

	FloatV4 operator*(const FloatV4 & a, const FloatV4 & b);

	FloatV4 operator/(const FloatV4 & a, const FloatV4 & b);


	FloatV4 MulAdd(const FloatV4 & a, const FloatV4 & b, const FloatV4 & c);


	FloatV4 Abs(const FloatV4 & value);

	FloatV4 Sign(const FloatV4 & value);

	FloatV4 Invert(const FloatV4 & value);


	FloatV4 RoundNearest(const FloatV4 & value);

	FloatV4 RoundDown(const FloatV4 & value);


	FloatV4 Reciprocal(const FloatV4 & value);

	FloatV4 LinearInterpolate(const FloatV4 & x, const FloatV4 & in0, const FloatV4 & in1);

	FloatV4 Normalise(const FloatV4 & value, const FloatV4 & in0, const FloatV4 & in1);


	FloatV4 Min(const FloatV4 & a, const FloatV4 & b);

	FloatV4 Max(const FloatV4 & a, const FloatV4 & b);

	FloatV4 ClipNormal(const FloatV4 & value);

	BoolV4 Inside(const FloatV4 & value, const FloatV4 & start, const FloatV4 & length);


	FloatV4 Modulo(const FloatV4 & a, const FloatV4 & b);


	FloatV4 Exp(const FloatV4 & value);

	FloatV4 Log(const FloatV4 & value);


	FloatV4 Exp2(const FloatV4 & value);

	FloatV4 Log2(const FloatV4 & value);


	FloatV4 Pow(const FloatV4 & x, const FloatV4 & y);

	FloatV4 SquareRoot(const FloatV4 & x);

}

REFLEX_SET_TRAIT(SIMD::FloatV4, IsRawComparable);




//
//impl

REFLEX_NS(Reflex::SIMD)

extern const FloatV4 kfMinusOne;

extern const FloatV4 kfZero;

extern const FloatV4 kfOne;

extern const FloatV4 f_m_zero;
extern const FloatV4 kSigns[16];

extern const FloatV4 kfRcp2;

extern const FloatV4 kExp2_max;
extern const FloatV4 kExp2_min;

extern const FloatV4 _ps_exp_hi, _ps_exp_lo;
extern const FloatV4 _ps_cephes_LOG2EF;
extern const FloatV4 _ps_cephes_exp_C1, _ps_cephes_exp_C2;
extern const FloatV4 _ps_cephes_exp_p0, _ps_cephes_exp_p1, _ps_cephes_exp_p2, _ps_cephes_exp_p3, _ps_cephes_exp_p4, _ps_cephes_exp_p5;

extern const Int32x4 kExpBase;

extern const Int32x4 kLog2_exp;
extern const Int32x4 kLog2_mant;

extern const Float32x4 kSignMaskV4;

REFLEX_END

REFLEX_INLINE Reflex::SIMD::BoolV4 Reflex::SIMD::operator==(const FloatV4 & a, const FloatV4 & b)
{
	return Reinterpret<BoolV4>(REFLEX_EQ_F32_X4(a, b));
}

REFLEX_INLINE Reflex::SIMD::BoolV4 Reflex::SIMD::operator!=(const FloatV4 & a, const FloatV4 & b)
{
	return Reinterpret<BoolV4>(REFLEX_IEQ_F32_X4(a, b));
}

REFLEX_INLINE Reflex::SIMD::BoolV4 Reflex::SIMD::operator>(const FloatV4 & a, const FloatV4 & b)
{
	return Reinterpret<BoolV4>(REFLEX_GT_F32_X4(a,b));
}

REFLEX_INLINE Reflex::SIMD::BoolV4 Reflex::SIMD::operator>=(const FloatV4 & a, const FloatV4 & b)
{
	return Reinterpret<BoolV4>(REFLEX_GTEQ_F32_X4(a, b));
}

REFLEX_INLINE Reflex::SIMD::BoolV4 Reflex::SIMD::operator<(const FloatV4 & a, const FloatV4 & b)
{
	return Reinterpret<BoolV4>(REFLEX_LT_F32_X4(a, b));
}

REFLEX_INLINE Reflex::SIMD::BoolV4 Reflex::SIMD::operator<=(const FloatV4 & a, const FloatV4 & b)
{
	return Reinterpret<BoolV4>(REFLEX_LTEQ_F32_X4(a, b));
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::operator-(const FloatV4 & value)
{
	return Reinterpret<FloatV4>(REFLEX_XOR_F32_X4(value, f_m_zero));
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::operator+(const FloatV4 & a, const FloatV4 & b)
{
	return Reinterpret<FloatV4>(REFLEX_ADD_F32_X4(a, b));
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::operator-(const FloatV4 & a, const FloatV4 & b)
{
	return Reinterpret<FloatV4>(REFLEX_SUB_F32_X4(a, b));
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::operator*(const FloatV4 & a, const FloatV4 & b)
{
	return Reinterpret<FloatV4>(REFLEX_MUL_F32_X4(a, b));
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::operator/(const FloatV4 & a, const FloatV4 & b)
{
	return Reinterpret<FloatV4>(REFLEX_DIV_F32_X4(a, b));
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::MulAdd(const FloatV4 & a, const FloatV4 & b, const FloatV4 & c)
{
	return Reinterpret<FloatV4>(REFLEX_MULADD_F32_X4(a, b, c));
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::RoundNearest(const FloatV4 & v)
{
	return Reinterpret<FloatV4>(REFLEX_I32_X4_TO_F32_X4(REFLEX_F32_X4_TO_I32_X4(v.data)));
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::RoundDown(const FloatV4 & v)
{
	return Reinterpret<FloatV4>(REFLEX_I32_X4_TO_F32_X4(REFLEX_TRUNCATE_F32_X4(v.data)));
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::Sign(const FloatV4 & v)
{
	return kSigns[REFLEX_I32_X4_TO_I32_MASK(Reinterpret<Int32x4>(v.data))];
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::Invert(const FloatV4 & v)
{
	return Reinterpret<FloatV4>(REFLEX_MUL_F32_X4(v, kfMinusOne));	//TODO OPTIMISE
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::Abs(const FloatV4 & value)
{
	return Reinterpret<FloatV4>(REFLEX_ANDNOT_F32_X4(kSignMaskV4, value));
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::Reciprocal(const FloatV4 & value)
{
	return Reinterpret<FloatV4>(REFLEX_RCP_F32_X4(value));
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::LinearInterpolate(const FloatV4 & x, const FloatV4 & in0, const FloatV4 & in1)
{
	return (x * (in1 - in0)) + in0;
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::Normalise(const FloatV4 & in, const FloatV4 & in0, const FloatV4 & in1)
{
	return (in - in0) / (in1 - in0);
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::Min(const Reflex::SIMD::FloatV4 & a, const Reflex::SIMD::FloatV4 & b)
{
	return Reinterpret<FloatV4>(REFLEX_MIN_F32_X4(a, b));
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::Max(const FloatV4 & a, const FloatV4 & b)
{
	return Reinterpret<FloatV4>(REFLEX_MAX_F32_X4(a, b));
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::ClipNormal(const FloatV4 & value)
{
	return Max(Min(value, kfOne), kfZero);
}

REFLEX_INLINE Reflex::SIMD::BoolV4 Reflex::SIMD::Inside(const FloatV4 & value, const FloatV4 & start, const FloatV4 & length)
{
	return And(value >= start, value < (start + length));
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::Modulo(const FloatV4 & a, const FloatV4 & b)
{
	FloatV4 c = a - (RoundDown(a / b) * b);

	return c + Select(c < kfZero, b);
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::Exp(const FloatV4 & value)
{
	Float32x4 one = kfOne;

	Float32x4 x = REFLEX_MIN_F32_X4(value, _ps_exp_hi);

	x = REFLEX_MAX_F32_X4(x, _ps_exp_lo);


	auto fx = REFLEX_MUL_F32_X4(x, _ps_cephes_LOG2EF);

	fx = REFLEX_ADD_F32_X4(fx, kfRcp2);


	auto emm0 = REFLEX_TRUNCATE_F32_X4(fx);

	auto tmp = REFLEX_I32_X4_TO_F32_X4(emm0);


	/* if greater, substract 1 */
	auto mask = Reinterpret<Float32x4>(REFLEX_GT_F32_X4(tmp, fx));

	mask = REFLEX_AND_F32_X4(mask, one);

	fx = REFLEX_SUB_F32_X4(tmp, mask);


	tmp = REFLEX_MUL_F32_X4(fx, _ps_cephes_exp_C1);

	auto z = REFLEX_MUL_F32_X4(fx, _ps_cephes_exp_C2);

	x = REFLEX_SUB_F32_X4(x, tmp);

	x = REFLEX_SUB_F32_X4(x, z);


	z = REFLEX_MUL_F32_X4(x, x);

	auto y = _ps_cephes_exp_p0.data;
	y = REFLEX_MUL_F32_X4(y, x);
	y = REFLEX_ADD_F32_X4(y, _ps_cephes_exp_p1.data);
	y = REFLEX_MUL_F32_X4(y, x);
	y = REFLEX_ADD_F32_X4(y, _ps_cephes_exp_p2.data);
	y = REFLEX_MUL_F32_X4(y, x);
	y = REFLEX_ADD_F32_X4(y, _ps_cephes_exp_p3.data);
	y = REFLEX_MUL_F32_X4(y, x);
	y = REFLEX_ADD_F32_X4(y, _ps_cephes_exp_p4.data);
	y = REFLEX_MUL_F32_X4(y, x);
	y = REFLEX_ADD_F32_X4(y, _ps_cephes_exp_p5.data);
	y = REFLEX_MUL_F32_X4(y, z);
	y = REFLEX_ADD_F32_X4(y, x);
	y = REFLEX_ADD_F32_X4(y, one);

	/* build 2^n */
	emm0 = REFLEX_TRUNCATE_F32_X4(fx);

	emm0 = REFLEX_ADD_I32_X4(emm0, kExpBase);

	emm0 = REFLEX_SHL1_I32_X4(emm0, 23);

	auto pow2n = Reinterpret<Float32x4>(emm0);

	return Reinterpret<FloatV4>(REFLEX_MUL_F32_X4(y, pow2n));
}

#define VEC_POLY0(x, c0) REFLEX_SET1_F32_X4(c0)
#define VEC_POLY1(x, c0, c1) REFLEX_ADD_F32_X4(REFLEX_MUL_F32_X4(VEC_POLY0(x, c1), x), REFLEX_SET1_F32_X4(c0))
#define VEC_POLY2(x, c0, c1, c2) REFLEX_ADD_F32_X4(REFLEX_MUL_F32_X4(VEC_POLY1(x, c1, c2), x), REFLEX_SET1_F32_X4(c0))
#define VEC_POLY3(x, c0, c1, c2, c3) REFLEX_ADD_F32_X4(REFLEX_MUL_F32_X4(VEC_POLY2(x, c1, c2, c3), x), REFLEX_SET1_F32_X4(c0))
#define VEC_POLY4(x, c0, c1, c2, c3, c4) REFLEX_ADD_F32_X4(REFLEX_MUL_F32_X4(VEC_POLY3(x, c1, c2, c3, c4), x), REFLEX_SET1_F32_X4(c0))
#define VEC_POLY5(x, c0, c1, c2, c3, c4, c5) REFLEX_ADD_F32_X4(REFLEX_MUL_F32_X4(VEC_POLY4(x, c1, c2, c3, c4, c5), x), REFLEX_SET1_F32_X4(c0))
#define REFLEX_EXP_PRECISION 4

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::Exp2(const FloatV4 & value)
{
	auto x = REFLEX_MAX_F32_X4(REFLEX_MIN_F32_X4(value.data, kExp2_max.data), kExp2_min.data);

	auto ipart = REFLEX_F32_X4_TO_I32_X4(REFLEX_SUB_F32_X4(x, kfRcp2.data));

	auto fpart = REFLEX_SUB_F32_X4(x, REFLEX_I32_X4_TO_F32_X4(ipart));

	auto t0 = REFLEX_ADD_I32_X4(ipart, kExpBase);

	auto expipart = Reinterpret<Float32x4>(REFLEX_SHL1_I32_X4(t0, 23));

	//minimax polynomial fit of 2**x, in range [-0.5, 0.5[
	#if REFLEX_EXP_PRECISION == 5
	auto expfpart = VEC_POLY5(fpart, 9.9999994e-1f, 6.9315308e-1f, 2.4015361e-1f, 5.5826318e-2f, 8.9893397e-3f, 1.8775767e-3f);
	#elif REFLEX_EXP_PRECISION == 4
	auto expfpart = VEC_POLY4(fpart, 1.0000026f, 6.9300383e-1f, 2.4144275e-1f, 5.2011464e-2f, 1.3534167e-2f);
	#elif REFLEX_EXP_PRECISION == 3
	auto expfpart = VEC_POLY3(fpart, 9.9992520e-1f, 6.9583356e-1f, 2.2606716e-1f, 7.8024521e-2f);
	#elif REFLEX_EXP_PRECISION == 2
	auto expfpart = VEC_POLY2(fpart, 1.0017247f, 6.5763628e-1f, 3.3718944e-1f);
	#else
	#error
	#endif

	return Reinterpret<FloatV4>(REFLEX_MUL_F32_X4(expipart, expfpart));
}

#define REFLEX_LOG_PRECISION 4

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::Log2(const FloatV4 & value)
{
	auto i = Reinterpret<Int32x4>(value);

	auto e = REFLEX_I32_X4_TO_F32_X4(REFLEX_SUB_I32_X4(REFLEX_SHR1_I32_X4(REFLEX_AND_I32_X4(i, kLog2_exp), 23), kExpBase));

	auto m = REFLEX_OR_F32_X4(Reinterpret<Float32x4>(REFLEX_AND_I32_X4(i, kLog2_mant)), kfOne);

	/* Minimax polynomial fit of log2(x)/(x - 1), for x in range [1, 2[ */
	#if REFLEX_LOG_PRECISION == 6
	auto p = VEC_POLY5( m, 3.1157899f, -3.3241990f, 2.5988452f, -1.2315303f,  3.1821337e-1f, -3.4436006e-2f);
	#elif REFLEX_LOG_PRECISION == 5
	auto p = VEC_POLY4(m, 2.8882704548164776201f, -2.52074962577807006663f, 1.48116647521213171641f, -0.465725644288844778798f, 0.0596515482674574969533f);
	#elif REFLEX_LOG_PRECISION == 4
	auto p = VEC_POLY3(m, 2.61761038894603480148f, -1.75647175389045657003f, 0.688243882994381274313f, -0.107254423828329604454f);
	#elif REFLEX_LOG_PRECISION == 3
	auto p = VEC_POLY2(m, 2.28330284476918490682f, -1.04913055217340124191f, 0.204446009836232697516f);
	#else
	#error
	#endif

	p = REFLEX_MUL_F32_X4(p, REFLEX_SUB_F32_X4(m, kfOne.data));

	return Reinterpret<FloatV4>(REFLEX_ADD_F32_X4(p, e));
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::Pow(const FloatV4 & x, const FloatV4 & y)
{
   return Exp2(REFLEX_MUL_F32_X4(Log2(x).data, y.data));
}

REFLEX_INLINE Reflex::SIMD::FloatV4 Reflex::SIMD::SquareRoot(const FloatV4 & x)
{
	return Reinterpret<FloatV4>(REFLEX_SQRT_F32_X4(x.data));
}

#include "../../../include/reflex/simd.h"




//
//sse global data

REFLEX_BEGIN_INTERNAL(Reflex::SIMD)

constexpr Int32 kSignMask = 0x80000000;

REFLEX_END_INTERNAL

const Reflex::SIMD::BoolV4 Reflex::SIMD::kTrueV4(kBooleanTrue);

const Reflex::SIMD::BoolV4 Reflex::SIMD::kFalseV4(kBooleanFalse);

const Reflex::SIMD::FloatV4 Reflex::SIMD::kfMinusOne(-1.0f);

const Reflex::SIMD::FloatV4 Reflex::SIMD::kfZero(0.0f);

const Reflex::SIMD::FloatV4 Reflex::SIMD::kfOne(1.0f);

const Reflex::SIMD::IntV4 Reflex::SIMD::kiZero(0);

const Reflex::SIMD::IntV4 Reflex::SIMD::kiOne(1);

const Reflex::SIMD::Float32x4 Reflex::SIMD::kSignMaskV4 = REFLEX_SET1_F32_X4(Reinterpret<Reflex::Float>(kSignMask));

const Reflex::SIMD::FloatV4 Reflex::SIMD::kSigns[16] =
{
	{ 1.0f, 1.0f, 1.0f, 1.0f },
	{ -1.0f, 1.0f, 1.0f, 1.0f },
	{ 1.0f, -1.0f, 1.0f, 1.0f },
	{ -1.0f, -1.0f, 1.0f, 1.0f },
	{ 1.0f, 1.0f, -1.0f, 1.0f },
	{ -1.0f, 1.0f, -1.0f, 1.0f },
	{ 1.0f, -1.0f, -1.0f, 1.0f },
	{ -1.0f, -1.0f, -1.0f, 1.0f },
	{ 1.0f, 1.0f, 1.0f, -1.0f },
	{ -1.0f, 1.0f, 1.0f, -1.0f },
	{ 1.0f, -1.0f, 1.0f, -1.0f },
	{ -1.0f, -1.0f, 1.0f, -1.0f },
	{ 1.0f, 1.0f, -1.0f, -1.0f },
	{ -1.0f, 1.0f, -1.0f, -1.0f },
	{ 1.0f, -1.0f, -1.0f, -1.0f },
	{ -1.0f, -1.0f, -1.0f, -1.0f },
};

const Reflex::SIMD::FloatV4 Reflex::SIMD::f_m_zero(Reinterpret<Float>(2147483648UL));	//set last bit

const Reflex::SIMD::FloatV4 Reflex::SIMD::kfRcp2(0.5f);

const Reflex::SIMD::FloatV4 Reflex::SIMD::_ps_exp_hi(88.3762626647949f);
const Reflex::SIMD::FloatV4 Reflex::SIMD::_ps_exp_lo(-88.3762626647949f);
const Reflex::SIMD::FloatV4 Reflex::SIMD::_ps_cephes_LOG2EF(1.44269504088896341f);
const Reflex::SIMD::FloatV4 Reflex::SIMD::_ps_cephes_exp_C1(0.693359375f);
const Reflex::SIMD::FloatV4 Reflex::SIMD::_ps_cephes_exp_C2(-2.12194440e-4f);
const Reflex::SIMD::FloatV4 Reflex::SIMD::_ps_cephes_exp_p0(1.9875691500E-4f);
const Reflex::SIMD::FloatV4 Reflex::SIMD::_ps_cephes_exp_p1(1.3981999507E-3f);
const Reflex::SIMD::FloatV4 Reflex::SIMD::_ps_cephes_exp_p2(8.3334519073E-3f);
const Reflex::SIMD::FloatV4 Reflex::SIMD::_ps_cephes_exp_p3(4.1665795894E-2f);
const Reflex::SIMD::FloatV4 Reflex::SIMD::_ps_cephes_exp_p4(1.6666665459E-1f);
const Reflex::SIMD::FloatV4 Reflex::SIMD::_ps_cephes_exp_p5(5.0000001201E-1f);

const Reflex::SIMD::FloatV4 Reflex::SIMD::kExp2_max(129.00000f);
const Reflex::SIMD::FloatV4 Reflex::SIMD::kExp2_min(-126.99999f);
const Reflex::SIMD::Int32x4 Reflex::SIMD::kExpBase = Reflex::SIMD::IntV4(0x0000007F);   // 127

const Reflex::SIMD::Int32x4 Reflex::SIMD::kLog2_exp = Reflex::SIMD::IntV4(0x7F800000);
const Reflex::SIMD::Int32x4 Reflex::SIMD::kLog2_mant = Reflex::SIMD::IntV4(0x007FFFFF);

const Reflex::Int Reflex::SIMD::Detail::kCount[16] = {0,1,1,2, 1,2,2,3, 1,2,2,3, 2,3,3,4};

const Reflex::UInt Reflex::SIMD::Detail::kFree[16] = {0,1,0,2, 0,1,0,3, 0,1,0,2, 0,1,0,4};

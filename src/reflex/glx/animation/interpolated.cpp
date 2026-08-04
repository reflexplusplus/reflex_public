#include "../../../../include/reflex/glx/animation/interpolated.h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

Float Linear(Float x)
{
	return x;
}

Float EaseIn2x(Float x)
{
	return Square(x);
}

Float EaseIn3x(Float x)
{
	return Cube(x);
}

Float EaseOut2x(Float x)
{
	return 1.0f - Square(1.0f - x);
}

Float EaseOut3x(Float x)
{
	return 1.0f - Cube(1.0f - x);
}

Float EaseInOutCos(Float x)
{
	return 0.5f - 0.5f * std::cos(x * kPif);
}

Float EaseInOut2x(Float x)
{
	Float32 sqr = Square(x);

	return sqr / (2.0f * (sqr - x) + 1.0f);
}

REFLEX_END_INTERNAL

Reflex::GLX::InterpolatedAnimation::Easing Reflex::GLX::Detail::GetEasing(Key32 id, InterpolatedAnimation::Easing default_idx)
{
	return InterpolatedAnimation::Easing(GetEasingCurveEx(id, default_idx) - kEasings);
}

const Reflex::Pair <Reflex::Key32,Reflex::GLX::Detail::EasingFn> Reflex::GLX::Detail::kEasings[] =
{
	{ K32("linear"), &GLX::Linear },

	{ K32("ease_in"), &GLX::EaseIn2x },
	{ K32("ease_in_cubic"), &GLX::EaseIn3x },

	{ K32("ease_out"), &GLX::EaseOut2x },
	{ K32("ease_out_cubic"), &GLX::EaseOut3x },

	{ K32("ease_in_out_cos"), &GLX::EaseInOutCos },
	{ K32("ease_in_out"), &GLX::EaseInOut2x },
};

Reflex::Float32 Reflex::GLX::Detail::InvertEasing(InterpolatedAnimation::Easing easing, Float32 y)
{
	switch (easing)
	{
	case InterpolatedAnimation::kLinear:
		return y;

	case InterpolatedAnimation::kEaseOut2x:
		return (1.0f - SquareRoot(1.0f - y));

	case InterpolatedAnimation::kEaseIn2x:
		return SquareRoot(y);

	case InterpolatedAnimation::kEaseOut3x:
		return (1.0f - Cube(1.0f - y));

	case InterpolatedAnimation::kEaseIn3x:
		return std::cbrt(y);

	case InterpolatedAnimation::kEaseInOut2x:
	{
		Float a = 2.0f * y - 1.0f;

		// A will never be 0 in (0,1); if y == 0.5, A == 0, handle that case
		if (a == 0.0f) return 0.5f;

		Float b = 2.0f * y;

		Float c = 4.0f * y * (1.0f - y); // simplified discriminant

		Float a2_rcp = 1.0f / (2.0f * a);

		Float sqrt = SquareRoot(c);

		Float x1 = (b + sqrt) * a2_rcp;

		Float x2 = (b - sqrt) * a2_rcp;

		// Return the root in [0,1]

		return (x1 >= 0.0f && x1 <= 1.0f) ? x1 : x2;
	}

	default:
		return y;
	}
}

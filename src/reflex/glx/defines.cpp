#include "../../../include/reflex/glx/defines.h"
#include "../../../include/reflex/glx/functions.h"
#include "../../../include/reflex/glx/detail/defines.h"




//
//defines

const Reflex::Pair <Reflex::GLX::Orientation> Reflex::GLX::Detail::kAlignmentToOrientation[kNumAlignment] =
{
	{ GLX::kOrientationNear, GLX::kOrientationNear },
	{ GLX::kOrientationCenter, GLX::kOrientationNear },
	{ GLX::kOrientationFar, GLX::kOrientationNear },

	{ GLX::kOrientationNear, GLX::kOrientationCenter },
	{ GLX::kOrientationCenter, GLX::kOrientationCenter },
	{ GLX::kOrientationFar, GLX::kOrientationCenter },

	{ GLX::kOrientationNear, GLX::kOrientationFar },
	{ GLX::kOrientationCenter, GLX::kOrientationFar },
	{ GLX::kOrientationFar, GLX::kOrientationFar },
};

Reflex::Output Reflex::GLX::output("GLX");

const Reflex::SIMD::FloatV4 Reflex::GLX::Detail::kRoundingToleranceV4 = Core::kRoundingTolerance;

const Reflex::SIMD::BoolV4 Reflex::GLX::Detail::kClipX = { SIMD::kBooleanTrue, SIMD::kBooleanFalse, SIMD::kBooleanTrue, SIMD::kBooleanFalse };

const Reflex::SIMD::BoolV4 Reflex::GLX::Detail::kClipY = { SIMD::kBooleanFalse, SIMD::kBooleanTrue, SIMD::kBooleanFalse, SIMD::kBooleanTrue };

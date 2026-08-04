#pragma once

#include "animation.h"




//
//Secondary API (Primary: CreateXXXAnimation)

namespace Reflex::GLX
{

	class InterpolatedAnimation;

}




//
//InterpolatedAnimation

class Reflex::GLX::InterpolatedAnimation : public Animation
{
public:

	REFLEX_OBJECT(GLX::InterpolatedAnimation, Animation);

	static InterpolatedAnimation & null;



	//types

	enum Easing : UInt8
	{
		kLinear,

		kEaseIn2x,
		kEaseIn3x,

		kEaseOut2x,
		kEaseOut3x,

		kEaseInOutCos,
		kEaseInOut2x,

		kNumEasing,
	};



	//lifetime
	
	using Animation::Animation;



	//setup

	void SetEasing(Easing easing) { OnSetEasing(easing); }

	void Flip() { OnFlip(); }


	
protected:

	//callbacks

	virtual void OnSetEasing(Easing easing) = 0;

	virtual void OnFlip() = 0;

};

REFLEX_SET_TRAIT(Reflex::GLX::InterpolatedAnimation, IsSingleThreadExclusive)




//
//impl

REFLEX_NS(Reflex::GLX::Detail)

InterpolatedAnimation::Easing GetEasing(Key32 id, InterpolatedAnimation::Easing default_idx = InterpolatedAnimation::kLinear);

Float32 InvertEasing(InterpolatedAnimation::Easing easing, Float32 y);

using EasingFn = FunctionPointer <Float(Float)>;

extern const Pair <Key32,EasingFn> kEasings[InterpolatedAnimation::kNumEasing];

REFLEX_END
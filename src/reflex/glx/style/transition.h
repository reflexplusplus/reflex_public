#pragma once

#include "reflex/glx/object.h"
#include "reflex/glx/animation/interpolated.h"




//
//

REFLEX_NS(Reflex::GLX)

inline const Pair <Key32, Detail::EasingFn> * GetEasingCurveEx(Key32 id, InterpolatedAnimation::Easing default_idx = InterpolatedAnimation::kLinear)
{
	return SearchValue<KeyCompare>(ToView(Detail::kEasings), id, Detail::kEasings + default_idx);
}

struct StateTransition : public Animation
{
	REFLEX_OBJECT(StateTransition, Animation);


	static constexpr auto kTransitionCurve = K32("transition_curve");


	StateTransition(GLX::Object & object, const Style & from, const Style & to, Float time);

	void OnBegin() override {}

	bool OnClock(Float delta) override;

	void OnSkip() override;



	Core::WeakReference m_object;

	ConstReference <Style> m_to;


	Key32 m_styleid;

	InterpolatedAnimation::Easing m_easing;

	Float m_time_rcp, x;
};

REFLEX_END
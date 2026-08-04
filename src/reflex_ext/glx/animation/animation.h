#pragma once

#include "../../../../include/reflex_ext/glx/animation/functions.h"




//
//ObjectAnimation

REFLEX_NS(Reflex::GLX)

struct ObjectAnimation : public Animation
{
protected:

	using Animation::Animation;

	void OnSetTarget(GLX::Object & target) override { m_target = target; }


	Core::WeakReference m_target;
};

struct InterpolatedAnimationImpl : public InterpolatedAnimation
{
public:

	typedef FunctionPointer <void(InterpolatedAnimationImpl&,GLX::Object&,Float)> InterpolateFn;

	InterpolatedAnimationImpl(InterpolateFn interpolatefn);



protected:

	void OnSetTarget(GLX::Object & target) override { m_target = target; }

	virtual Float32 OnRecomputeProgress(GLX::Object & target) { return 0.0f; }


	Core::WeakReference m_target;



private:

	void OnSetTime(Float32 time) final;

	void OnSetEasing(Easing curve) final;

	void OnBegin() final;

	bool OnClock(Float delta) final;

	void OnSkip() final;


	Easing m_easing;

	Detail::EasingFn m_easing_fn;

	InterpolateFn m_interpolatefn;

	Float m_time, m_time_rcp, m_remain;
};

REFLEX_INLINE Float32 SafeNormalise(Float32 value, Float32 from, Float32 to)
{
	auto range = to - from;

	return range ? ((value - from) / range) : 0.0f;
}

REFLEX_END

#pragma once

#include "[require].h"




//
//Internal

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

inline void RunAnimation(Object & object, Key32 id, Float32 time, TRef <Animation> animation)
{
	object.SetProperty(id, animation);

	animation->SetTarget(object);

	animation->SetTime(time);

	animation->Play();
}

struct AnimationPolyfill : Animation
{
	AnimationPolyfill(const Function <void(GLX::Object & target, Float x)> & callback)
		: m_callback(callback)
	{
	}

	void OnSetTarget(GLX::Object & object) override { m_target = object; }

	bool OnClock(Float delta) override
	{
		m_callback(m_target, 1.0f);

		m_callback.Clear();

		return true;
	}

	void OnSkip() override
	{
		m_callback(m_target, 1.0f);
	}


	Core::WeakReference m_target;

	Function <void(GLX::Object & target, Float x)> m_callback;
};

REFLEX_END_INTERNAL

REFLEX_NS(Reflex::GLX)
extern FunctionPointer <TRef <Animation>(Float, Float, const Function <void(Object&, Float)> &, Float)> g_create_logarithmic_animation;
extern FunctionPointer <TRef <Animation>(const Function <void(Object &, Float)> &)> g_create_interpolated_animation;
REFLEX_END

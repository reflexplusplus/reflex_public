#include "animation.h"
#include "reflex/glx/functions/state.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

struct NonAnimatedStateAnimation : public ObjectAnimation
{
	NonAnimatedStateAnimation(Key32 state)
		: m_state(state)
	{
	}

	~NonAnimatedStateAnimation()
	{
		GLX::AnimationScope scope(false);

		m_target->UnsetState(m_state);
	}

	void OnBegin() override
	{
		m_target->SetState(m_state);
	}

	bool OnClock(Float delta) override { return false; }

	void OnSkip() override {}


	const Key32 m_state;
};

struct StateAnimation : public NonAnimatedStateAnimation
{
	StateAnimation(Key32 state)
		: NonAnimatedStateAnimation(state),
		m_time(0.0f),
		m_remain(0.0f)
	{
	}

	~StateAnimation()
	{
		m_target->UnsetState(m_state);
	}

	void OnSetTime(Float32 time) override
	{
		m_time = time;
	}

	void OnBegin() override
	{
		m_remain = m_time;

		m_target->SetState(m_state);
	}

	bool OnClock(Float delta) override
	{
		m_remain -= delta;

		if (m_remain <= 0.0f)
		{
			m_remain = m_time + m_remain;

			ToggleState(m_target, m_state);
		}

		return true;
	}

	Float m_time, m_remain;
};

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::GLX::Animation> Reflex::GLX::CreateCallbackAnimation(const Function <void(Object&)> & callback)
{
	struct Callback : public ObjectAnimation
	{
		Callback(const Function <void(GLX::Object&)> & callback)
			: m_callback(callback),
			m_time(0.0f),
			m_remain(0.0f),
			m_called(false)
		{
		}

		void OnSetTime(Float32 time) override
		{
			m_time = time;
		}

		void OnBegin() override 
		{
			m_remain = m_time;

			m_called = false;
		}

		bool OnClock(Float delta) override 
		{
			m_remain -= delta;

			if (m_remain <= 0.0f)
			{
				if (SetFiltered(m_called, true))
				{
					m_callback(m_target);
				}

				return false;
			}
			else
			{
				return true;
			}
		}

		void OnSkip() override 
		{ 
			if (SetFiltered(m_called, true))
			{
				m_callback(m_target);
			}
		}


		Function <void(GLX::Object&)> m_callback;

		Float m_time, m_remain;

		bool m_called;
	};

	return REFLEX_CREATE(Callback, callback);
}

Reflex::TRef <Reflex::GLX::Animation> Reflex::GLX::CreateStateAnimation(Key32 state)
{
	if (AnimationScope::IsEnabled())
	{
		return REFLEX_CREATE(StateAnimation, state);
	}
	else
	{
		return REFLEX_CREATE(NonAnimatedStateAnimation, state);
	}
}

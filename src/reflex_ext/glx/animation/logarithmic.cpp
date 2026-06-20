#include "animation.h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

static constexpr Float32 kDefaultDecayFactor = 0.003f;

struct AbstractLogarithmicTransition : public ObjectAnimation
{
	AbstractLogarithmicTransition(Float from, Float to, Float decay_factor);

	virtual void OnSetTime(Float32 time) override 
	{
		m_speed_rcp = 1.0f / Max(time, Core::kRoundingTolerance);
	}

	virtual void OnProcess(GLX::Object & target, Float32 x) = 0;

	virtual void OnBegin() override;

	virtual bool OnClock(Float delta) override;

	virtual void OnSkip() override;



protected:

	const Float32 m_decay_factor;

	const Float32 m_from, m_to;

	Float m_epsilon;

	Float m_speed_rcp;

	Float m_x;
};

AbstractLogarithmicTransition::AbstractLogarithmicTransition(Float from, Float to, Float decay_factor)
	: m_decay_factor(Clip(decay_factor, Core::kRoundingTolerance, 1.0f - Core::kRoundingTolerance))
	, m_from(from)
	, m_to(to)
	, m_speed_rcp(2.0f)
{
	auto range = Abs(m_to - m_from);

	auto epsilon = Clip(range * 0.001f, 0.00001f, Detail::kPixelSize);

	m_epsilon = epsilon / range;
}

void AbstractLogarithmicTransition::OnBegin()
{
	m_x = 1.0f;

	OnProcess(m_target, m_from);
}

bool AbstractLogarithmicTransition::OnClock(Float delta)
{
	m_x *= (delta ? Pow(m_decay_factor, delta * m_speed_rcp) : 1.0f);

	if (m_x < m_epsilon)
	{
		m_x = 0.0f;

		OnProcess(m_target, m_to);

		return false;
	}
	else
	{
		auto value = Reflex::LinearInterpolate(m_x, m_to, m_from);
		
		OnProcess(m_target, value);

		return true;
	}
}

void AbstractLogarithmicTransition::OnSkip()
{
	OnProcess(m_target, m_to);
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::GLX::Animation> Reflex::GLX::CreateMaxBoundsAnimation(Key32 id, bool yaxis, Float from, Float to)
{
	struct LogarithmicResize : public AbstractLogarithmicTransition
	{
		REFLEX_INLINE LogarithmicResize(Key32 id, bool y, Float from, Float to)
			: AbstractLogarithmicTransition(from, to, kDefaultDecayFactor),
			m_id(id),
			m_yaxis(y)
		{
		}
		
		void OnProcess(GLX::Object & object, Float value) override
		{
			Size min;

			Size max = kLarge;

			auto size = RoundNearest(value);

			//Detail::SetSize(m_yaxis, min, size);

			Detail::SetSize(m_yaxis, max, size);

			SetBounds(object, m_id, min, max);
		}

		const Key32 m_id;

		bool m_yaxis;
	};

	if (from != to)
	{
		return REFLEX_CREATE(LogarithmicResize, id, yaxis, from, to);
	}
	else
	{
		return Animation::null;
	}
}

Reflex::TRef <Reflex::GLX::Animation> Reflex::GLX::Detail::CreateZoomAnimation(Key32 id, Float from, Float to)
{
	static constexpr auto UnsetMagnification = [](Object & object, Key32 id)
	{
		object.ClearMod(Reflex::Detail::MergeHashes(id.value, Detail::kmagnification));
	};

	static constexpr auto SetMagnification = [](Object & object, Key32 id, Float scale)
	{
		object.SetMod(Reflex::Detail::MergeHashes(id.value, Detail::kmagnification), Detail::ComputedStyle::Create(scale, 1.0f, Detail::ComputedStyle::kRenderAuto));
	};

	struct LogarithmicZoom : public AbstractLogarithmicTransition
	{
		REFLEX_INLINE LogarithmicZoom(Key32 id, Float from, Float to)
			: AbstractLogarithmicTransition(from, to, kDefaultDecayFactor),
			m_id(id)
		{
		}

		virtual void OnProcess(GLX::Object & object, Float value) override
		{
			SetMagnification(object, m_id, value);

			if (value == m_to)
			{
				if (m_to == 1.0f)
				{
					UnsetMagnification(object, m_id);
				}
				else
				{
					SetMagnification(object, m_id, m_to);
				}
			}
		}

		const Key32 m_id;
	};

	if (from != to)
	{
		return REFLEX_CREATE(LogarithmicZoom, id, from, to);
	}
	else
	{
		return Animation::null;
	}
}

Reflex::TRef <Reflex::GLX::Animation> Reflex::GLX::CreateLogarithmicAnimation(Float from, Float to, const Function <void(Object&, Float)> & callback, Float decay_factor)
{
	struct LogarithmicTransition : public AbstractLogarithmicTransition
	{
		REFLEX_INLINE LogarithmicTransition(Float from, Float to, Float decay_factor, const Function <void(GLX::Object&,Float)> & callback)
			: AbstractLogarithmicTransition(from, to, decay_factor),
			m_callback(callback)
		{
		}

		virtual void OnProcess(GLX::Object & object, Float value) override { m_callback(object, value); }

		Function <void(GLX::Object&, Float)> m_callback;
	};

	return REFLEX_CREATE(LogarithmicTransition, from, to, decay_factor, callback);
}

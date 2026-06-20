#include "animation.h"
#include "reflex/glx/functions/mods.h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

struct OpacityAnimation : public InterpolatedAnimationImpl
{
	OpacityAnimation(Key32 id, Float from, Float to, Detail::ComputedStyle::Render render)
		: InterpolatedAnimationImpl(reinterpret_cast<InterpolateFn>(&OnInterpolate)),
		m_id(id),
		m_render(render),
		m_from(from),
		m_difference(to - from)
	{
	}

	void OnSetTarget(GLX::Object & target) override
	{
		InterpolatedAnimationImpl::OnSetTarget(target);

		//m_render = target.GetMod(Reflex::Detail::MergeHashes(m_id.value, kopacity))->GetRender();
	}

	void OnFlip() override
	{
		auto from = m_from;

		auto to = m_from + m_difference;

		Swap(from, to);

		m_from = from;

		m_difference = (to - from);
	}

	Float32 OnRecomputeProgress(GLX::Object & target) override
	{
		return SafeNormalise(GetOpacity(target, m_id), m_from, (m_from + m_difference));
	}

	static void OnInterpolate(OpacityAnimation & self, GLX::Object & target, Float x)
	{
		SetOpacity(target, self.m_id, self.m_from + (self.m_difference * x), self.m_render);
	}


	Key32 m_id;

	Detail::ComputedStyle::Render m_render;

	Float m_from, m_difference;
};

REFLEX_END_INTERNAL

Reflex::GLX::InterpolatedAnimationImpl::InterpolatedAnimationImpl(InterpolateFn interpolatefn)
	: m_easing(kLinear)
	, m_easing_fn(Detail::kEasings[kLinear].b)
	, m_interpolatefn(interpolatefn)
	, m_time(0.0f)
	, m_time_rcp(0.0f)
	, m_remain(0.0f)
{
}

void Reflex::GLX::InterpolatedAnimationImpl::OnSetTime(Float time)
{
	m_time = time;

	m_time_rcp = time ? (1.0f / time) : 0.0f;
}

void Reflex::GLX::InterpolatedAnimationImpl::OnSetEasing(Easing easing)
{
	m_easing = easing;

	m_easing_fn = Detail::kEasings[easing].b;
}

void Reflex::GLX::InterpolatedAnimationImpl::OnBegin()
{
	m_remain = (1.0f - Detail::InvertEasing(m_easing, OnRecomputeProgress(m_target))) * m_time;
}

bool Reflex::GLX::InterpolatedAnimationImpl::OnClock(Float delta)
{
	m_remain -= delta;

	if (m_remain > 0.0f)
	{
		auto x = 1.0f - (m_remain * m_time_rcp);

		//REFLEX_ASSERT(x >= 0.0f && x <= 1.0f);

		m_interpolatefn(*this, m_target, m_easing_fn(x));

		return true;
	}
	else
	{
		m_interpolatefn(*this, m_target, m_easing_fn(1.0f));

		return false;
	}
}

void Reflex::GLX::InterpolatedAnimationImpl::OnSkip()
{
	m_interpolatefn(*this, m_target, m_easing_fn(1.0f));
}

Reflex::TRef <Reflex::GLX::InterpolatedAnimation> Reflex::GLX::CreateInterpolatedAnimation(const Function <void(Object&,Float)> & callback)
{
	struct Callback : public InterpolatedAnimationImpl
	{
		Callback(const Function <void(GLX::Object&,Float)> & callback)
			: InterpolatedAnimationImpl(reinterpret_cast<InterpolateFn>(&OnInterpolate))
			, m_callback(callback)
			, m_flipped(0)
		{
		}

		static void OnInterpolate(Callback & self, GLX::Object & object, Float x)
		{
			if (self.m_flipped & 1)
			{
				self.m_callback(object, 1.0f - x);
			}
			else
			{
				self.m_callback(object, x);
			}
		}

		void OnFlip() override
		{
			m_flipped++;
		}


		Function <void(GLX::Object&,Float)> m_callback;

		UInt8 m_flipped;
	};

	return REFLEX_CREATE(Callback, callback);
}

Reflex::TRef <Reflex::GLX::InterpolatedAnimation> Reflex::GLX::CreateWaitAnimation()
{
	struct Wait : public InterpolatedAnimationImpl
	{
		Wait()
			: InterpolatedAnimationImpl(reinterpret_cast<InterpolateFn>(&OnInterpolate))
		{
		}
		
		static void OnInterpolate(Wait & self, GLX::Object & object, Float x)
		{
		}

		void OnFlip() override {}
	};

	return REFLEX_CREATE(Wait);
}

Reflex::TRef <Reflex::GLX::InterpolatedAnimation> Reflex::GLX::CreateOpacityAnimation(Key32 id, Float from, Float to, Detail::ComputedStyle::Render render)
{
	if (from != to)
	{
		return REFLEX_CREATE(OpacityAnimation, id, from, to, render);
	}
	else
	{
		return InterpolatedAnimation::null;
	}
}

Reflex::TRef <Reflex::GLX::InterpolatedAnimation> Reflex::GLX::CreatePositionAnimation(bool y, Float from, Float to)
{
	struct AxisMoveAnimation : public InterpolatedAnimationImpl
	{
		AxisMoveAnimation(bool y, Float from, Float to)
			: InterpolatedAnimationImpl(reinterpret_cast<InterpolateFn>(&OnInterpolate)),
			m_yaxis(y),
			m_start(from),
			m_difference(to - from)
		{
		}

		void OnSetTarget(GLX::Object & target) final
		{
			REFLEX_ASSERT(GLX::Detail::GetPositioning(target).a == GLX::Detail::kPositioningAbsolute);

			InterpolatedAnimationImpl::OnSetTarget(target);
		}

		Float32 OnRecomputeProgress(GLX::Object & object) override
		{
			return SafeNormalise(Detail::GetPoint(m_yaxis, object.GetRect().origin), m_start, (m_start + m_difference));
		}

		static void OnInterpolate(AxisMoveAnimation & self, GLX::Object & object, Float x)
		{
			auto axis = Detail::SnapToPixels(self.m_start + (self.m_difference * x));

			bool yaxis = self.m_yaxis;

			auto xy = Detail::MakePoint(yaxis, axis, Detail::GetPoint(!yaxis, object.GetRect().origin));

			object.SetPosition(xy);
		}

		void OnFlip() override
		{
			auto cur = Detail::GetPoint(m_yaxis, m_target->GetRect().origin);

			auto start = m_start;

			m_start = cur;
			
			m_difference = start - cur;
		}

		bool m_yaxis;

		Float m_start, m_difference;
	};

	if (from != to)
	{
		return REFLEX_CREATE(AxisMoveAnimation, y, from, to);
	}
	else
	{
		return InterpolatedAnimation::null;
	}
}

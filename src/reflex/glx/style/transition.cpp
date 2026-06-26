#include "transition.h"
#include "detail/cstyle.h"




//
//

Reflex::GLX::StateTransition::StateTransition(GLX::Object & object, const Style & from, const Style & to, Float time)
	: m_object(object),
	m_to(to),
	m_styleid(to.id),
	m_easing(Detail::GetEasing(Data::GetKey32(to, kTransitionCurve), InterpolatedAnimation::kEaseOut2x)),
	m_time_rcp(Reflex::Reciprocal(time)),
	x(0.0f)
{
	REFLEX_ASSERT(time);

	if (auto t = DynamicCast<Detail::ComputedStyleTransition>(m_object->GetMod(kMaxUInt32)))
	{
		auto pstate = &t->transition;

		if (pstate->m_styleid == from.id)
		{
			//compensate, by negating the new_ curve, so that when applied it matches the current actual value

			auto current = 1.0f - Detail::kEasings[pstate->m_easing].b(pstate->x);

			x = Detail::InvertEasing(m_easing, current);
		}
	}
}

bool Reflex::GLX::StateTransition::OnClock(Float delta)
{
	x += (delta * m_time_rcp);

	if (x < 1.0f)
	{
		if (auto t = DynamicCast<Detail::ComputedStyleTransition>(m_object->GetMod(kMaxUInt32)))
		{
			RemoveConst(t)->Morph();

			Core::Accessor::CallComputeStyle(m_object);

			auto cstyle = m_object->GetComputedStyle();

			Core::Accessor::CallSetRenderer(m_object, cstyle->CreateRenderer(m_object), cstyle->GetZIndex());

			m_object->Accommodate();
		}

		return true;
	}
	else
	{
		m_object->UnsetMod(kMaxUInt32);

		return false;
	}
}

void Reflex::GLX::StateTransition::OnSkip()
{
	x = 1.0f;

	m_object->UnsetMod(kMaxUInt32);
}

#include "implementation.h"
#include "../../../../../include/reflex/glx/detail/functions.h"




//
//content

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

const Float ViewPortImpl::AutoScroll::kSensitivity = 48.0f;

const Float ViewPortImpl::AutoScroll::kInterval = 0.75f;

ViewPortImpl::AutoScroll::AutoScroll(AbstractViewPort * pscroller, bool scoped, Float amount)
	: scroller(*pscroller),
	m_onclock(Core::desktop->CreateAnimationClock(BindMethod(this, &AutoScroll::OnClock, _P1))),
	m_amount(amount),
	m_edge(kNumAlignment),
	m_scoped(scoped)
{
}

void ViewPortImpl::AutoScroll::OnClock(Float delta)
{
	//TODO ("this doesnt support body margin, rather use GetAbs(GetContent()->GetParent())");

	auto [vo,vr] = scroller.GetView();

	auto size_pixels = Reinterpret<Size>(vr / scroller.GetPixelsPerUnit());

	if (auto pointer = Detail::QueryPointer(scroller.GetWindow()))
	{
		Point pointer_position = TransformPosition(scroller, pointer->position);

		if (Or(m_scoped, Contains(scroller.GetRect().size, pointer_position)))
		{
			auto range = scroller.GetExtent();

			bool x = vr.w < range.w;

			bool y = vr.h < range.h;

			if (x && (pointer_position.x < kSensitivity))
			{
				SetEdge(kAlignmentLeft);
			}
			else if (x && (pointer_position.x > (size_pixels.w - kSensitivity)))
			{
				SetEdge(kAlignmentRight);
			}
			else if (y && (pointer_position.y < kSensitivity))
			{
				SetEdge(kAlignmentTop);
			}
			else if (y && (pointer_position.y > (size_pixels.h - kSensitivity)))
			{
				SetEdge(kAlignmentBottom);
			}
			else
			{
				SetEdge(kAlignmentCenter);
			}
		}
		else
		{
			SetEdge(kAlignmentCenter);
		}

		m_remainder -= delta;

		if (m_remainder <= 0.0f)
		{
			switch (m_edge)
			{
			case kAlignmentLeft:
				scroller.Reveal(false, vo.x - RoundNearest(vr.w * m_amount), vr.w, kSensitivity);
				break;

			case kAlignmentRight:
				scroller.Reveal(false, vo.x + RoundNearest(vr.w * m_amount), vr.w, kSensitivity);
				break;

			case kAlignmentTop:
				scroller.Reveal(true, vo.y - RoundNearest(vr.h * m_amount), vr.h, kSensitivity);
				break;

			case kAlignmentBottom:
				scroller.Reveal(true, vo.y + RoundNearest(vr.h * m_amount), vr.h, kSensitivity);
				break;
			}

			m_remainder = kInterval;
		}
	}
	else
	{
		SetEdge(kAlignmentCenter);
	}
}

REFLEX_INLINE void ViewPortImpl::AutoScroll::SetEdge(Alignment edge)
{
	if (SetFiltered(m_edge, edge)) m_remainder = kInterval;
}

REFLEX_END_INTERNAL

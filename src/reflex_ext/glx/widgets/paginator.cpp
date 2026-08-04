#include "../../../../include/reflex_ext/glx/widgets/paginator.h"
#include "../../../../include/reflex_ext/glx/functions/enter_exit.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

REFLEX_END_INTERNAL

Reflex::GLX::Paginator::Paginator()
{
	m_show[0] = false;

	m_show[1] = false;

	SkipExit(m_prevnext[0], false);

	SkipExit(m_prevnext[1], false);

	//EnableMouse(false);
}

void Reflex::GLX::Paginator::OnSetStyle(const Style & style)
{
	AbstractViewBar::OnSetStyle(style);

	m_prevnext[0].SetStyle(style["prev"]);

	m_prevnext[1].SetStyle(style["next"]);
}

bool Reflex::GLX::Paginator::OnEvent(Object & src, Event & e)
{
	//REFLEX_ASSERT(!MouseEnabled().a);
	//REFLEX_ASSERT(!MouseEnabled().b);

	if (e.id == kMouseDown)
	{
		//TODO this should move to next visible item.  using size is a quick hack for Vice

		if (auto idx = Search(m_prevnext, *Cast<GLX::Button>(src)))
		{
			Float itemsize = Detail::GetSize(GetAxis(*this), /*item->*/GetRect().size);

			EmitJump(AbstractViewBar::region.start + (itemsize * (idx.value ? 1.0f : -1.0f)));

			return true;
		}
	}
	else if (e.id == kMouseWheel)
	{
		auto delta = GetMouseDelta(e);

		auto viewport = GetContainingViewPort(*this);

		bool y = GetAxis(*this);

		auto [vo,vr] = viewport->GetView();

		Detail::PerformStandardScroll(*this, Detail::GetPoint(y, vo), delta.y, y, false, GetModifierKeys(e) & kModifierKeyPrimary);

		return true;
	}

	return Object::OnEvent(src, e);
}

void Reflex::GLX::Paginator::SetFlow(UInt8 flowflags)
{
	Alignment alignments[] = { kAlignmentLeft, kAlignmentRight };

	if (flowflags & kFlowY)
	{
		alignments[0] = kAlignmentTop;
		
		alignments[1] = kAlignmentBottom;
	}

	auto palignments = alignments;

	for (auto & i : m_prevnext)
	{
		AddFloat(*this, i, *palignments++);
	}
}

void Reflex::GLX::Paginator::OnUpdate()
{
	bool shows[2] = { True(AbstractViewBar::region.start), AbstractViewBar::region.start < (AbstractViewBar::extent - AbstractViewBar::region.length) };

	Button * btn = m_prevnext;

	REFLEX_LOOP(idx, 2)
	{
		bool show = shows[idx];

		if (SetFiltered(m_show[idx], show))
		{
			if (show)
			{
				btn->SetParent(*this);

				Enter(*btn, kEnterAnimationFade);
			}
			else
			{
				Exit(*btn, true);
			}
		}

		btn++;
	}
}

Reflex::GLX::Trap Reflex::GLX::Paginator::OnMouseOver(Core::MouseAction mouseaction, UInt8 flags)
{
	//workaround because ViewPort enables/disables mouse
	
	return kTrapThru;
}

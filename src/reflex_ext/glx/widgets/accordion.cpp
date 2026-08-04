#include "../../../../include/reflex_ext/glx/widgets/accordion.h"
#include "../../../../include/reflex_ext/glx/functions/enter_exit.h"



//
//

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

REFLEX_DECLARE_KEY32(open);

void Reveal(Accordion & accordion)
{
	REFLEX_DECLARE_KEY32(entering);

	Object & body = accordion.body;

	if (auto viewport = GetContainingViewPort(accordion))
	{
		bool y = GetAxis(*viewport);

		auto bounds = GetBounds(body, kentering).b;

		UnsetBounds(body, kentering);

		auto contentsize = Detail::GetSize(y, Detail::ComputeContentSize(accordion));

		auto abs = CalculateAbs(viewport->GetContent(), accordion);

		viewport->Reveal(y, Detail::GetPoint(y, abs.a), contentsize, 0.0f);

		SetBounds(body, kentering, {}, bounds);
	}
}

REFLEX_END_INTERNAL

Reflex::GLX::Accordion::Accordion(bool open)
	: m_enterflags(kEnterAnimationFade | kEnterAnimationSize)
{
	header->SetMouseCursor(kMouseCursorPointer);

	EnableInline<true>(body, kOrientationFit);

	SkipEnter(body, m_enterflags);

	if (open)
	{
		header->SetState(kopen);
	}
	else
	{
		SkipExit(body, true);
	}

	//EnableClip();
}

void Reflex::GLX::Accordion::Clear()
{
	body->Clear();
}

void Reflex::GLX::Accordion::Open(bool animate, bool reveal)
{
	if (!IsOpen())
	{
		if (EmitRequest(*this, kAccordionOpen))
		{
			body->SetParent(*this);

			Refresh();	//for responsive content, need to make sure width is set first

			if (And(animate, true))
			{
				Enter(body, m_enterflags);

				if (reveal) Reveal(*this);
			}
			else
			{
				SkipEnter(body);
			}

			header->SetState(kopen);
		}
	}
}

void Reflex::GLX::Accordion::Close(bool animate)
{
	if (IsOpen())
	{
		if (EmitRequest(*this, kAccordionClose))
		{
			if (And(animate, true))
			{
				Exit(body, true);
			}
			else
			{
				SkipExit(body, true);
			}

			header->UnsetState(kopen);
		}
	}
}

void Reflex::GLX::Accordion::Toggle(bool animate)
{
	if (IsOpen())
	{
		Close(animate);
	}
	else
	{
		Open(animate);
	}
}

bool Reflex::GLX::Accordion::IsOpen() const
{
	if (body->GetParent())
	{
		return HasEntered(body);
	}
	else
	{
		return false;
	}
}

bool Reflex::GLX::Accordion::OnEvent(Object & src, Event & e)
{
	if (src == header && IsLeftClick(e))
	{
		Toggle(true);

		return true;
	}

	return Object::OnEvent(src, e);
}

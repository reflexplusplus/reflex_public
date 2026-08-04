#include "../../include/widgets/accordiongroup.h"
#include "../../include/animation.h"
#include "../../include/functions.h"




//
//implementation

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

struct CloseTransition
{
	REFLEX_DECLARE_KEY32(CloseTransition);

	CloseTransition(Object & object)
		: object(object),
		m_yaxis(GetAxis(object.GetParent()))
	{
		m_from = object.GetRect().size;
	}

	~CloseTransition()
	{
		Float to = Detail::GetSize(m_yaxis, object.ComputeLayout().contentsize);

		SetBounds(object, kCloseTransition, m_from);

		TRef multi = REFLEX_CREATE(PlayList);

		AddScene(multi, CreateResize(object, kCloseTransition, Detail::GetSize(m_yaxis, m_from), to));
		
		AddScene(multi, CreateCallback2(Bind(&GLX::UnsetBounds, kCloseTransition, _P1)));

		Run(object, kCloseTransition, multi);
	}

	Object & object;

	bool m_yaxis;

	Size m_from;
};

REFLEX_END_INTERNAL




//
//

REFLEX_NS(Reflex)

GLX::AccordionGroup::AccordionGroup()
	: m_cstyle(&Detail::Compile<ComputedStyle>(Style::null)),
	m_allowclose(false)
{
	SetClass(kObjectClass);

	SetAxis(true);

	EnableAutoFit(this, false, false);	//should fit to largest item

	EnableClip();
}

void GLX::AccordionGroup::SetIndex(UInt index, bool animate)
{
	Array < Reference <Accordion> > accordions;

	for (auto & i : *this) if (itr.kTypeID == Accordion::kClassID) accordions.Push(Cast<Accordion>(&i));

	if (index < accordions.GetSize())
	{
		if (Detail::EmitRequest<SelectMsg>(*this, index))
		{
			Event::Mute open(*this, Accordion::kAccordionOpen);

			m_allowclose = true;

			REFLEX_LOOP(idx, accordions.GetSize())
			{
				auto & accordion = *accordions[idx];

				Stop(accordion, CloseTransition::kCloseTransition);

				if (And(idx != index, accordion.IsOpen()))
				{
					if (animate)
					{
						{
							CloseTransition t(accordion);

							accordion.SetAlignment(kPositioningInline, kOrientationNear, kOrientationFit);

							accordion.Close(false);
						}

						accordion.Open(false, false);

						accordion.Close(true);
					}
					else
					{
						accordion.SetAlignment(kPositioningInline, kOrientationNear, kOrientationFit);

						accordion.Close(false);

						UnsetBounds(accordion, CloseTransition::kCloseTransition);
					}
				}
			}

			auto & accordion = *accordions[index];

			accordion.SetAlignment(kPositioningInline, kOrientationFit, kOrientationFit);

			accordion.Open(animate, false);

			UnsetBounds(accordion, CloseTransition::kCloseTransition);

			m_allowclose = false;
		}
	}
}

Idx GLX::AccordionGroup::GetIndex() const
{
	UInt idx = 0;

	for (auto & i : *this)
	{
		if (i.kTypeID == Accordion::kClassID)
		{
			if (Cast<Accordion>(i).IsOpen()) return idx;

			idx++;
		}
	}

	return -1;
}

void GLX::AccordionGroup::OnSetStyle(const Style & style)
{
	m_cstyle = &Detail::Compile<ComputedStyle>(style);
}

bool GLX::AccordionGroup::OnEvent(Object & src, Event & e)
{
	if (auto open = CastMsg<Accordion::OpenMsg>(e))
	{
		if (&src.GetParent() == this)
		{
			SetIndex(LookupIndex(src).value);
		}

		return true;
	}
	else if (auto close = CastMsg<Accordion::CloseMsg>(e))
	{
		if (&src.GetParent() == this)
		{
			close->p1 = m_allowclose;

			return true;
		}
	}

	return Object::OnEvent(src, e);
}

void GLX::AccordionGroup::SetLayout(Reflex::Object & object)
{
	auto & layout = Cast<Detail::LayoutData>(object);

	layout.layoutflags &= UInt8(~((1 << Detail::kLayoutInvert) | (1 << Detail::kLayoutCenter) | (1 << Detail::kLayoutWrap)));

	bool y = layout.layoutflags.Check(Detail::kLayoutY);

	for (auto & i : *this) if (itr.kTypeID == Accordion::kClassID) i.SetAxis(y);

	GLX::Object::OnSetLayout(object);
}

GLX::Accordion & GLX::AccordionGroup::AddItem()
{
	auto & accordion = AddInline(*this, Init(REFLEX_CREATE(Accordion, false), m_cstyle->item));

	accordion.SetAxis(GetAxis(*this));

	return accordion;
}




//
//style

GLX::AccordionGroup::ComputedStyle::ComputedStyle(const Style & style)
	: item(style.Retrieve(kItem))
{
}

REFLEX_END

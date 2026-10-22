#include "../../../../include/reflex_ext/glx/widgets/paginator.h"
#include "../../../../include/reflex_ext/glx/functions/enter_exit.h"




//
//

struct Reflex::GLX::Paginator::LayoutModel : public Detail::StandardLayout
{
	Pair <AccommodateFn, AlignFn> OnRebuild(GLX::Object & object, UInt8 layout_flags) override
	{
		auto base = Detail::StandardLayout::OnRebuild(object, layout_flags);

		auto self = Cast<Paginator>(object);

		Alignment alignments[] = { kAlignmentLeft, kAlignmentRight };

		if (object.GetLayoutFlags() & kFlowY)
		{
			alignments[0] = kAlignmentTop;
			alignments[1] = kAlignmentBottom;
		}

		auto palignments = alignments;

		for (auto & i : self->m_prevnext)
		{
			EnableFloat(i, *palignments++);
		}

		return base;
	}

	static LayoutModel st_self;
};

Reflex::GLX::Paginator::LayoutModel Reflex::GLX::Paginator::LayoutModel::st_self;

const Reflex::GLX::AbstractViewPort::ViewBarType Reflex::GLX::Paginator::kType = 
{ 
	[](AbstractViewPort&) -> Unretained <AbstractViewBar>
	{
		return REFLEX_CREATE(Paginator); 
	},
	[](bool y, const Style &) -> Detail::Positioning
	{
		return Detail::kPositioningFloat;
	} 
};

Reflex::GLX::Paginator::Paginator()
	: AbstractViewBar(LayoutModel::st_self)
{
	m_show[0] = false;
	m_show[1] = false;

	for (auto & i : m_prevnext)
	{
		i.SetParent(*this);

		SkipExit(i, false);
	}
}

void Reflex::GLX::Paginator::OnSetStyle(const Style & style)
{
	AbstractViewBar::OnSetStyle(style);

	m_prevnext[0].SetStyle(style["prev"]);

	m_prevnext[1].SetStyle(style["next"]);
}

bool Reflex::GLX::Paginator::OnEvent(Object & src, Event & e)
{
	if (e.id == kMouseDown)
	{
		//TODO this should move to next visible item.  using size is a quick hack for Vice

		if (auto idx = Search(m_prevnext, *Cast<GLX::Button>(src)))
		{
			Float itemsize = Detail::GetSize(GetAxis(*this), /*item->*/GetRect().size);

			EmitJump(GetRange().b.start + (itemsize * (idx.value ? 1.0f : -1.0f)));

			return true;
		}
	}
	else if (e.id == kMouseWheel)
	{
		auto delta = GetDelta(e);

		auto viewport = GetContainingViewPort(*this);

		bool y = GetAxis(*this);

		auto [vo,vr] = viewport->GetView();

		Detail::PerformStandardScroll(*this, Detail::GetPoint(y, vo), delta.y, y, false, GetModifierKeys(e) & kModifierKeyPrimary);

		return true;
	}

	return Object::OnEvent(src, e);
}

void Reflex::GLX::Paginator::OnUpdate()
{
	auto [extent, region] = GetRange();

	bool shows[2] = { True(region.start), region.start < (extent - region.length) };

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

Reflex::GLX::Core::Trap Reflex::GLX::Paginator::OnPointerTender(Core::PointerAction action, const Core::Pointer & pointer, UInt8 flags)
{
	//workaround because ViewPort enables/disables mouse
	
	return Core::kTrapThru;
}

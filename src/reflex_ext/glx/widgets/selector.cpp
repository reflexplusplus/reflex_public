#include "../../../../include/reflex_ext/glx/widgets/selector.h"
#include "../../../../include/reflex_ext/glx/detail/functions.h"




//
//impl

struct Reflex::GLX::Selector::LayoutModel : public Detail::StandardLayout
{
	Pair <AccommodateFn,AlignFn> OnRebuild(GLX::Object & object, UInt8 layout_flags) override
	{
		auto self = Cast<Selector>(object);

		auto base = StandardLayout::OnRebuild(object, layout_flags);

		base.a = CastAccommodateFn<Selector>(self->m_autofit ? &Selector::OnAccommodate<true> : &Selector::OnAccommodate<false>);

		return base;
	}
};

Reflex::GLX::Selector::Selector()
	: GLX::Object(New<LayoutModel>())
	, m_set_content(&Detail::SetContentNotAnimated)
	, m_autofit(false)
{
}

Reflex::GLX::Selector::~Selector()
{
	Clear();
}

void Reflex::GLX::Selector::EnableContentAutoFit(bool enable)
{
	m_autofit = enable;

	RebuildLayout();
}

void Reflex::GLX::Selector::Clear()
{
	UnsetProperty<Detail::StrongRefObject>(K32("Exchange"));

	m_index = {};

	Object::Clear();

	m_content.Clear();

	Accommodate();
}

void Reflex::GLX::Selector::AddPanel(TRef <Object> item, Key32 style_id)
{
	REFLEX_HEAPCHECK(output, this, AddPanel, *item);

	Detail::ApplySubStyle(item, GetStyle(), style_id);

	m_content.Push({ item, style_id });

	Accommodate();
}

void Reflex::GLX::Selector::RemovePanel(UInt idx)
{
	if (idx < m_content.GetSize())
	{
		auto & ref = *m_content[idx].a;

		ref.Detach();

		m_content.Remove(idx);

		SelectPanel(m_index.value - 1);

		Accommodate();
	}
}

void Reflex::GLX::Selector::SelectPanel(UInt idx)
{
	if (idx < m_content.GetSize())
	{
		bool far = idx > m_index.value;

		auto & item = *m_content[idx].a;

		if (SetFiltered<Idx>(m_index, idx))
		{
			m_set_content(*this, item, GetAxis(*this), far);

			auto e = Make<Event>(kSelectPanel);

			Data::SetUInt32(e, kindex, idx);

			REFLEX_DEBUG_WARN_SCOPE(not_on_heap, false);	//warnings already emitted when adding

			e->SetProperty(kitem, item);

			Emit(e);
		}
	}
}

void Reflex::GLX::Selector::OnSetStyle(const Style & style)
{
	GLX::Object::OnSetStyle(style);

	m_set_content = Detail::kSetContentSwitch[Detail::GetBool(style, kanimate)];

	for (auto & i : m_content)
	{
		Detail::ApplySubStyle(i.a, style, i.b);
	}
}

void Reflex::GLX::Selector::OnUpdate()
{
	for (auto & i : m_content) i.a->Update();
}

template <bool COMPUTE> void Reflex::GLX::Selector::OnAccommodate(Selector & self, bool & isresponsive, Size & contentsize)
{
	auto cstyle = self.GetComputedStyle();

	for (auto & i : self.m_content)
	{
		auto & item = *i.a;

		if constexpr (COMPUTE)
		{
			item.ComputeLayout();

			contentsize = Max(contentsize, item.contentsize + Sum(item.GetComputedStyle()->GetMargin()));

			isresponsive = isresponsive || item.isresponsive;
		}
		else
		{
			auto cstyle = Detail::Compile<Detail::ComputedStyle>(item.GetStyle());

			contentsize = Max(contentsize, cstyle->GetMinMax().a + Sum(cstyle->GetMargin()));
		}
	}

	contentsize += Sum(cstyle->GetPadding());

	auto & minmax = cstyle->GetMinMax();

	contentsize = Clip(contentsize, minmax.a, minmax.b);

	Detail::AccommodateRenderer(self, isresponsive, contentsize);
}

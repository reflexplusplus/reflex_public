#include "../../../../include/reflex_ext/glx/widgets/tabgroup.h"




//
//implementation

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

constexpr UInt8 kInlinePositioning = UInt8(kPositioningInline) | (UInt8(kOrientationNear) << 2) | (UInt8(kOrientationFit) << 4);
constexpr UInt8 kInlineFlexPositioning = UInt8(kPositioningInline) | (UInt8(kOrientationFit) << 2) | (UInt8(kOrientationFit) << 4);
constexpr UInt8 kTabPositioning[] = { kInlinePositioning, kInlinePositioning, kInlinePositioning, kInlineFlexPositioning };

REFLEX_END_INTERNAL

Reflex::GLX::TabGroup::TabGroup()
	: Form(New<Selector>()),
	m_content_alignment(kOrientationNear)
{
	SetFlow(body, kFlowX);
}

void Reflex::GLX::TabGroup::Clear()
{
	header->Clear();

	Cast<Selector>(body)->Clear();

	m_tabs.Clear();
}

void Reflex::GLX::TabGroup::OnSetStyle(const Style & style)
{
	constexpr UInt8 kHeaderFlowFlags[] = { kFlowX, kFlowCenter, kFlowInvert, kFlowX };

	Form::OnSetStyle(style);

	auto header_style = header->GetStyle();

	m_content_alignment = Detail::ParseOrientation(Data::GetKey32(header_style, K32("align_content")));

	SetFlow(header, kHeaderFlowFlags[m_content_alignment]);

	auto tab_positioning = Detail::kTabPositioning[m_content_alignment];

	for (auto & i : m_tabs)
	{
		auto & tab = *i.a;

		tab.SetPositioningFlags(tab_positioning);

		Detail::ApplySubStyle(tab, header_style, i.b);
	}
}

void Reflex::GLX::TabGroup::OnUpdate()
{
	body->Update();
}

bool Reflex::GLX::TabGroup::OnEvent(Object & src, Event & e)
{
	if (e.id == kMouseDown && src.GetParent() == header)
	{
		Cast<Selector>(body)->SelectPanel(LookupIndex(src).value);

		return true;
	}
	else if (e.id == Selector::kSelectPanel && &src == body.Adr())
	{
		auto index = GLX::GetIndex(e);

		if (index < m_tabs.GetSize())
		{
			for (auto & i : m_tabs) GLX::Select(i.a, false);

			GLX::Select(m_tabs[index].a);
		}

		Emit(e);

		return true;
	}
	else if (e.id == kKeyDown)
	{
		switch (GetKeyCode(e))
		{
		case kKeyCodeTab:
			if (auto focus = GetSelector()->GetCurrentIndex())
			{
				auto inc = (GetModifierKeys(e) & kModifierKeyShift) ? -1 : 1;

				focus = Modulo(Int(focus.value) + inc, Int(GetSelector()->GetNumPanel()));

				GetSelector()->SelectPanel(focus.value);
			}

			return true;
		}
	}

	return Object::OnEvent(src, e);
}

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::TabGroup::AddPanel(const WString::View & label, TRef <Object> content, Key32 style_id, Key32 tab_style_id)
{
	auto tab = REFLEX_CREATE(Button, label); 
	
	tab->SetStyle(header->GetStyle()[tab_style_id]);

	tab->SetPositioningFlags(Detail::kTabPositioning[m_content_alignment]);

	tab->SetParent(header);

	m_tabs.Push({ tab, tab_style_id });

	Cast<Selector>(body)->AddPanel(content, style_id);
	
	return tab;
}

void Reflex::GLX::TabGroup::RemovePanel(UInt idx)
{
	m_tabs[idx].a->Detach();

	m_tabs.Remove(idx);

	Cast<Selector>(body)->RemovePanel(idx);
}

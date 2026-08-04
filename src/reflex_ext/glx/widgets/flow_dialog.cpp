#include "../../../../include/reflex_ext/glx/widgets/flow_dialog.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

bool g_flow_dialog_selecting_page = false;

REFLEX_END_INTERNAL

Reflex::GLX::FlowDialog::FlowDialog(const WString::View & title)
	: FormEx(title)
{
	EnableAutoFit(body, false, false);

	EnableTabNavigation(*this);

	EnableFocusHighlight(*this);

	constexpr Alignment alignments[2] = { kAlignmentLeft, kAlignmentRight };

	REFLEX_LOOP(idx, 2)
	{
		AddFloat(footer, m_buttons[idx], alignments[idx]);
	}

	Clear();
}

void Reflex::GLX::FlowDialog::Clear()
{
	body->Clear();

	m_pages.Clear();

	m_content = {};

	UpdateButtons({});
}

void Reflex::GLX::FlowDialog::RegisterPage(Key32 id, Key32 prev, Key32 next, Key32 style_id, const Labels & labels, Function <TRef<Object>(FlowDialog&)> ctr)
{
	m_pages.Set(id, { .labels = labels, .prev_id = prev, .next_id = next, .style_id = style_id, .ctr = ctr });
}

void Reflex::GLX::FlowDialog::UpdateButton(Key32 id, Direction next, Key32 pageid)
{
	if (auto page = QueryPage(id))
	{
		GetAdr(page->prev_id)[next] = pageid;

		if (id == m_current_page)
		{
			UpdateButtons(*page);
		}
	}
}

void Reflex::GLX::FlowDialog::UpdateButtons(const Page & page)
{
	Function <void()> function;

	REFLEX_LOOP(next, 2)
	{
		const Key32 id = GetAdr(page.prev_id)[next];

		TRef button = m_buttons[next];

		if (auto pstep = QueryPage(id))
		{
			function = [this, next, id]()
			{
				ShowPage(id, Direction(next));
			};
		}
		else if (id == kZeroKey)
		{
			function = BindMethod(this, &FlowDialog::Close);
		}
		else
		{
			function.Clear();
		}

		button->id = id;

		SetText(button, GetAdr(page.labels.prev)[next]);

		BindClick(button, function);

		bool active = True(function);

		Activate(button, active);

		Data::SetBool(button, kWantsFocus, active);
	}
}

bool Reflex::GLX::FlowDialog::ShowPage(Key32 id, Direction fwd)
{
	if (auto page = QueryPage(id))
	{
		REFLEX_ASSERT(SetFiltered(g_flow_dialog_selecting_page, true));

		m_current_page = id;

		auto content = page->ctr(*this);

		m_current_style = page->style_id;

		Detail::ApplySubStyle(content, m_body_style, m_current_style);

		UpdateButtons(*page);

		Detail::SetContent(body, content, GetAxis(body), True(fwd));

		BeginEventForwarding(*this, kKeyDown, content);

		FocusBranch(content);

		m_content = content;

		auto e = Make<Event>(kShowPage);

		Data::SetKey32(e, kid, id);

		Emit(e);

		g_flow_dialog_selecting_page = false;

		return true;
	}
	else
	{
		return false;
	}
}

Reflex::GLX::FlowDialog::Page * Reflex::GLX::FlowDialog::QueryPage(Key32 page_id)
{
	return m_pages.Search(page_id);
}

void Reflex::GLX::FlowDialog::OnSetStyle(const Style & style)
{
	FormEx::OnSetStyle(style);

	m_body_style = body->GetStyle();

	auto footer_style = footer->GetStyle();

	m_buttons[0].SetStyle(footer_style["prev"]);

	m_buttons[1].SetStyle(footer_style["next"]);

	Detail::ApplySubStyle(m_content, m_body_style, m_current_style);
}

bool Reflex::GLX::FlowDialog::OnEvent(Object & src, Event & e)
{
	if (e.id == kKeyDown)
	{
		switch (GetKeyCode(e))
		{
		case kKeyCodeSpace:
		case kKeyCodeEnter:
			REFLEX_LOOP(idx, 2)
			{
				auto & button = m_buttons[idx];

				if (src == button)
				{
					e.id = kNullKey;

					ShowPage(button.id, Direction(idx));

					return true;	//dont trap the key, then char will be forwarded to text inputs
				}
			}
			break;

		default:
			break;
		}
	}

	return FormEx::OnEvent(src, e);
}

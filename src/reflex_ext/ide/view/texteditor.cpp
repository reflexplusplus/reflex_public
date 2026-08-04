#include "../globalimpl.h"




//
//

Reflex::IDE::Detail::TextEditor::TextEditor()
	: m_text(REFLEX_CREATE(GLX::Text, true))
{
	GLX::SetColourCanvas(*this, {}, [this](GLX::ColourCanvasContext & ctx)
	{
		constexpr auto kYellow = GLX::RGB(255, 255, 0);
		//constexpr auto kBlue = GLX::RGB(80, 192, 248,128);

		if (m_highlighted_line /*|| m_commented_lines*/)
		{
			GLX::AddRectFill(ctx.output, kYellow, GetLineCoordinates(m_highlighted_line.value));

			//auto [offset, unused] = m_textedit->GetLineCoordinates(0);

			//offset = GLX::Detail::SnapToPixels(offset * 0.5f) + 2.0f;

			//auto lineh = m_textedit->GetLineHeight();

			//for (auto idx : m_commented_lines)
			//{
			//	GLX::AddRectFill(points, kBlue, { { 0.0f, offset + (idx * lineh) }, { size.w, lineh } });
			//}
		}
	});

	SetProperty(GLX::kvalue, m_text);

	m_textedit = GLX::TextEditBehaviour::Create();

	SetDelegate(GLX::TextEditBehaviour::kTextEdit, m_textedit);

	GLX::SetFlow(*this, GLX::kFlowY);
}

void Reflex::IDE::Detail::TextEditor::ClearData()
{
	m_data.Clear();

	m_text->ClearValue();

	m_highlighted_line = {};

	Realign();
}

void Reflex::IDE::Detail::TextEditor::SetData(ConstTRef <Data::ArchiveObject> archive)
{
	if (m_data.Adr() != archive.Adr())
	{
		m_data = archive;

		GLX::SetText(*this, Data::DecodeUTF8(m_data->value));
	}
}

void Reflex::IDE::Detail::TextEditor::ClearError()
{
	m_highlighted_line = {};

	//GLX::SetState(this, GLX::kSelectedState, false);
}

void Reflex::IDE::Detail::TextEditor::Reveal(UInt position, UInt length)
{
	m_textedit->SetCaret(position, position, length);

	m_textedit->Reveal();

	Realign();
}

//void Reflex::IDE::Detail::TextEditor::SetCommentedLines(const ArrayView <UInt> & lines)
//{
//	m_commented_lines = lines;
//
//	Realign();
//}

void Reflex::IDE::Detail::TextEditor::SetError(UInt idx)
{
	//SetState(GLX::kSelectedState);

	if (SetFiltered<Idx>(m_highlighted_line, idx))
	{
		auto line = GetLineCoordinates(idx);

		GLX::GetContainingViewPort(*this)->Reveal(true, line.origin.y, line.size.h, 32.0f);

		auto textview = m_text->GetView();

		auto start = textview.data;

		while (textview)
		{
			auto line = Data::Detail::ReadLine(textview);

			if (!idx--)
			{
				Reveal(UInt(line.data - start), 0);

				break;
			}
		}

		Realign();
	}
}

bool Reflex::IDE::Detail::TextEditor::OnEvent(GLX::Object & src, GLX::Event & e)
{
	if (e.id == GLX::kTransaction)
	{
		if (GLX::GetTransactionStage(e) == GLX::kTransactionPerform)
		{
			auto utf8 = Data::EncodeUTF8(GLX::GetText(*this));

			if (utf8 != m_data->value)
			{
				m_data = REFLEX_CREATE(Data::ArchiveObject, std::move(utf8));

				GLX::Emit(GetParent(), kEdit);
			}

			return true;
		}
	}
	else if (e.id == GLX::kKeyDown)
	{
		switch (GLX_KEY_CODE(GLX::GetKeyCode(e), GLX::GetModifierKeys(e)))
		{
		case GLX_KEY_CODE(GLX::kKeyCodeO, GLX::kModifierKeyAlt):
			return false;	//prevent alt+o being delegated to textedit, which captures all 'alt' keys
		}
	}

	return GLX::Object::OnEvent(src, e);
}

Reflex::GLX::Rect Reflex::IDE::Detail::TextEditor::GetLineCoordinates(UInt idx) const
{
	auto line = m_textedit->GetLineCoordinates(idx);

	GLX::Rect rect = { {0.0, line.a}, { GetRect().size.w, line.b } };

	return GLX::Expand(rect, line.b);
}

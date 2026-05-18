#include "view.h"




//
//NotesCppApp::View implementation

namespace NotesCppApp { namespace {	//begin internal namespace

class ViewImpl : public View
{
public:

	ViewImpl(App & app);

	~ViewImpl();



private:

	//Bootstrap::View state-persistence callbacks

	void OnResetState(Key32 context) override;

	void OnRestoreState(Data::Archive::View & stream, Key32 context) override;

	void OnStoreState(Data::Archive & stream) const override;


	
	//GLX::Object callbacks

	bool OnEvent(GLX::Object & source, GLX::Event & e) override;

	void OnSetStyle(const GLX::Style & style) override;

	void OnUpdate() override;



	//links

	const TRef <App> app;



	//state

	UInt m_selection;



	//elements

	GLX::Object m_header;

	GLX::Button m_new, m_open, m_save;


	GLX::Split m_body;


	GLX::Object m_left;

	GLX::List m_list;

	GLX::Scroller m_list_scroller;

	GLX::Button m_add_note;


	GLX::TextArea m_textarea;


	GLX::Button m_ide;


	static constexpr UInt16 kChunkVersion = 1;
};

ViewImpl::ViewImpl(App & app)
	: View(app, kChunkVersion, L":res:NotesCppApp/styles.glx"),
	app(app),
	m_selection(0),
	m_new(L"New"),
	m_open(L"Open"),
	m_save(L"Save"),
	m_add_note(L"Add Note"),
	m_ide(L"Console"),
	m_textarea(true)
{
	Data::SetBool(*this, GLX::kresize, true);

	Data::SetBool(m_left, GLX::kresize, true);

	GLX::SetFlow(m_left, GLX::kFlowY);


	GLX::AddInline(m_header, m_new, GLX::kOrientationCenter);

	GLX::AddInline(m_header, m_open, GLX::kOrientationCenter);

	GLX::AddInline(m_header, m_save, GLX::kOrientationCenter);

	if constexpr (REFLEX_DEBUG) GLX::AddFloat(m_header, m_ide, GLX::kAlignmentRight);


	GLX::AddInline(*this, m_header);


	m_list_scroller.SetContent(m_list);

	GLX::AddInlineFlex(m_left, m_list_scroller);

	GLX::AddInline(m_left, m_add_note);

	GLX::AddInline(m_body, m_left);

	GLX::AddInlineFlex(m_body, m_textarea);

	GLX::AddInlineFlex(*this, m_body);
}

ViewImpl::~ViewImpl()
{
	//cover case that transaction never ended
	app->SetNote(m_selection, Data::EncodeUTF8(m_textarea.GetText()));
}

void ViewImpl::OnResetState(Key32 context)
{
	m_selection = 0;
}

void ViewImpl::OnRestoreState(Data::Archive::View & stream, Key32 context)
{
	Data::Deserialize(stream, m_selection);

	m_body.SetSplitSize(m_left, Data::Deserialize<Float32>(stream));
}

void ViewImpl::OnStoreState(Data::Archive & stream) const
{
	//if you change/add data here, you will need to increment the chunkversion, as previous chunks will be compatible

	Data::Serialize(stream, m_selection, m_body.GetSplitSize(m_left));
}

bool ViewImpl::OnEvent(GLX::Object & src, GLX::Event & e)
{
	constexpr auto ShowContextMenu = [](ViewImpl & self, const GLX::Event & e, UInt idx)
	{
		if (GLX::GetClickFlags(e) & GLX::kClickFlagRmb)
		{
			auto menu = GLX::OpenContextMenu(self);

			GLX::BindClick(menu->AddItem(L"Delete"), [&self, idx]()
			{
				self.app->DeleteNote(idx);
			});
		}
	};

	constexpr auto Open = [](App & app)
	{
		if (auto path = GLX::ShowFileDialog(false, { App::kFileExt }, app.GetFilename()))
		{
			app.Open(path);
		}
	};

	constexpr auto Save = [](App & app)
	{
		if (auto path = GLX::ShowFileDialog(true, { App::kFileExt }, app.GetFilename()))
		{
			app.Save(path);
		}
	};

	if (e.id == GLX::kTransaction)
	{
		if (GLX::GetTransactionStage(e) == GLX::kTransactionEnd)
		{
			app->SetNote(m_selection, Data::EncodeUTF8(m_textarea.GetText()));
		}

		return true;
	}
	else if (e.id == GLX::List::kListSelect)
	{
		if (Data::GetBool(e, GLX::kstate))
		{
			m_selection = GLX::GetIndex(e);

			if (auto antecedent = GLX::QueryAntecedent(e, GLX::kMouseDown))
			{
				ShowContextMenu(*this, *antecedent, m_selection);
			}

			Update();
		}

		return true;
	}
	else if (e.id == GLX::List::kListRequestRemove)
	{
		app->DeleteNote(GLX::GetIndex(e));

		return true;
	}
	else if (e.id == GLX::kMouseDown)
	{
		if (src == m_new)
		{
			app->Reset();
		}
		else if (src == m_open)
		{
			Open(app);
		}
		else if (src == m_save)
		{
			Save(app);
		}
		else if (src == m_add_note)
		{
			m_selection = app->GetNumNotes();

			app->AddNote();
		}
		else if (src.GetParent() == m_list)
		{
			ShowContextMenu(*this, e, GLX::LookupIndex(src).value);
		}
		else if constexpr (REFLEX_DEBUG)
		{
			if (src == m_ide) Bootstrap::global->EnableIde(!Bootstrap::global->IdeEnabled());
		}

		return true;
	}
	else if (e.id == GLX::kKeyDown)
	{
		auto modifiers = GLX::GetModifierKeys(e);

		if (modifiers & GLX::kModifierKeyPrimary)
		{
			switch (GLX::GetKeyCode(e))
			{
			case GLX::kKeyCodeN:
				app->Reset();
				return true;

			case GLX::kKeyCodeO:
				Open(app);
				return true;

			case GLX::kKeyCodeS:
				Save(app);
				return true;
			}
		}
	}

	return Bootstrap::View::OnEvent(src, e);
}

void ViewImpl::OnSetStyle(const GLX::Style & style)
{
	auto button = style["Button"];

	auto list = style["List"];


	m_body.SetStyle(style["Body"]);


	m_new.SetStyle(button);

	m_open.SetStyle(button);

	m_save.SetStyle(button);

	m_header.SetStyle(style["Header"]);


	m_list.SetStyle(list["content"]);

	m_list_scroller.SetStyle(list);

	m_add_note.SetStyle(button);

	m_left.SetStyle(style["Left"]);

	m_textarea.SetStyle(style["TextArea"]);


	if constexpr (REFLEX_DEBUG) m_ide.SetStyle(button);


	Update();
}

void ViewImpl::OnUpdate()
{
	auto item_style = GetStyle()["List"]["Item"];


	auto filename = File::SplitFilename(app->GetFilename()).b;

	WString display = filename ? filename : ToView(L"Untitled");

	if (app->IsEdited()) display.Append(L" *");

	GLX::SetText(m_header, display);


	GLX::RedirectFocus(m_list, m_list);	//preserve focus within m_list before m_list.Clear()
		
	m_list.Clear();

	if (auto n = app->GetNumNotes())
	{
		m_selection = Min<UInt>(m_selection, n - 1);

		for (UInt32 idx = 0; idx < n; ++idx)
		{
			auto item = GLX::AddInline(m_list, GLX::Init(New<GLX::Button>(ToWString(idx + 1)), item_style));

			GLX::Select(item, idx == m_selection);
		}

		m_textarea.SetText(Data::DecodeUTF8(app->GetNote(m_selection)));
	}
	else
	{
		m_textarea.ClearText();
	}


	if constexpr (REFLEX_DEBUG) GLX::Select(m_ide, Bootstrap::global->IdeEnabled());
}

} }

TRef <NotesCppApp::View> NotesCppApp::View::Create(App & app)
{
	return New<ViewImpl>(app);
}

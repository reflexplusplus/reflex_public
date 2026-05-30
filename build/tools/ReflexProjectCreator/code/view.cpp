#include "view.h"




namespace ReflexProjectCreator { namespace {	//begin internal namespace

class ViewImpl : public View
{
public:

	ViewImpl(App & app);


	const TemplateDefinition * GetCurrentTemplate() const;


	void OnResetState(Key32 context) override;

	void OnRestoreState(Data::Archive::View & stream, Key32 context) override;

	void OnStoreState(Data::Archive & stream) const override;


	void OnSetStyle(const GLX::Style & style) override;

	bool OnEvent(GLX::Object & source, GLX::Event & e) override;

	void OnUpdate() override;

	
	//void SetReflexPath();

	CString::View GetVariable(Key32 id) const;

	static TRef <GLX::Object> ParseMarkup(Data::Archive::View utf8, const GLX::Style & style);


	const TRef <App> app;

	WString m_current_template;

	Map <Key32,CString> m_properties;

	GLX::Object m_header_group;
	GLX::Object m_header_bar;
	GLX::Popup m_menu_popup;
	GLX::Object m_template_row;
	GLX::Popup m_template_popup;
	GLX::Object m_strings_group;
	GLX::Scroller m_info_scroller;
	GLX::Button m_build_button;
	//GLX::Object m_locate_group;
	//GLX::Object m_locate_label;
	//GLX::Button m_locate_button;
};

ViewImpl::ViewImpl(App & app)
	: View(app, 1, L":res:ReflexProjectCreator/styles.glx")
	, app(app)
	, m_build_button(L"Create")
	//, m_locate_button(L"Locate")
{
	Data::SetBool(*this, GLX::kresize, true);

	GLX::SetFlow(m_header_group, GLX::kFlowY);
	GLX::SetFlow(m_strings_group, GLX::kFlowY);

	GLX::AddFloat(m_header_bar, m_menu_popup, GLX::kAlignmentRight);
	GLX::AddInline(m_header_group, m_header_bar);
	GLX::AddInlineFlex(m_template_row, m_template_popup);
	GLX::AddInline(m_header_group, m_template_row);

	Data::SetBool(m_template_popup, GLX::kWantsFocus, true);
	Data::SetBool(m_build_button, GLX::kWantsFocus, true);

	GLX::AddInline(*this, m_header_group);
	GLX::AddInline(*this, m_strings_group);
	GLX::AddInlineFlex(*this, m_info_scroller);
	GLX::AddInline(*this, m_build_button);

	GLX::EnableFocusCycle(*this);
}

const TemplateDefinition * ViewImpl::GetCurrentTemplate() const
{
	struct IdCompare
	{
		static bool eq(const TemplateDefinition & tmpl, WString::View id)
		{
			return tmpl.folder == id;
		}
	};

	auto templates = app->GetTemplates();

	if (auto current = SearchValue<IdCompare>(templates, m_current_template))
	{
		return current;
	}
	else if (templates)
	{
		return &templates.GetFirst();
	}
	else
	{
		return nullptr;
	}
}

void ViewImpl::OnResetState(Key32 context)
{
}

void ViewImpl::OnRestoreState(Data::Archive::View & stream, Key32 context)
{
	Data::DeserializeUTF8(stream, m_current_template);

	Data::Deserialize(stream, m_properties);
}

void ViewImpl::OnStoreState(Data::Archive & stream) const
{
	Data::SerializeUTF8(stream, m_current_template);

	Data::Serialize(stream, m_properties);
}

void ViewImpl::OnSetStyle(const GLX::Style & style)
{
	auto group = style["Group"];

	m_header_group.SetStyle(group);
	m_strings_group.SetStyle(group);

	m_header_bar.SetStyle(style["Header"]);
	m_menu_popup.SetStyle(style["Header"]["Popup"]);
	m_template_row.SetStyle(style["Item"]);
	m_template_popup.SetStyle(style["Item"]["Popup"]);
	m_info_scroller.SetStyle(style["Scroller"]);
	m_build_button.SetStyle(style["Button"]);
	//m_locate_label.SetStyle(style["Locate"]);
	//m_locate_button.SetStyle(style["Button"]);

	GLX::SetText(m_template_row, L"Template");
	//GLX::SetText(m_locate_label, L"Please select the Reflex folder");

	Update();
}

bool ViewImpl::OnEvent(GLX::Object & source, GLX::Event & e)
{
	if (Bootstrap::View::OnEvent(source, e))
	{
		return true;
	}
	else if (e.id == GLX::kMouseDown)
	{
		if (source == m_build_button)
		{
			auto default_dest = Join(System::GetPath(System::kPathUserDocuments), L"Reflex Projects", System::kPathDelimiter);

			auto dest = GLX::ShowFolderDialog(Bootstrap::global->prefs, "project_creator.dest", true, default_dest, L"Select Destination");

			if (dest)
			{
				if (auto tmpl = GetCurrentTemplate())
				{
					Array <Pair<CString>> variables;

					for (auto & i : tmpl->strings)
					{
						variables.Push({ i.id, GetVariable(i.id) });
					}

					app->InstantiateTemplate(*tmpl, variables, dest);

					Update();
				}
			}

			return true;
		}
		//else if (source == m_locate_button)
		//{
		//	SetReflexPath();

		//	return true;
		//}
	}
	else if (auto menu = GLX::GetMenu(e))
	{
		if (source == m_menu_popup)
		{
			//GLX::BindClick(menu->AddItem(L"Set Reflex Installation Path"), [this]()
			//{
			//	SetReflexPath();
			//});

			GLX::BindClick(menu->AddItem(L"Reload Templates"), [this]()
			{
				app->RefreshTemplates();
			});

			return true;
		}
		else if (source == m_template_popup)
		{
			for (auto & tmpl : app->GetTemplates())
			{
				GLX::BindClick(menu->AddItem(ToWString(tmpl.name)), [this, folder = tmpl.folder]()
				{
					m_current_template = folder;

					Update();
				});
			}

			return true;
		}
	}

	return false;
}

void ViewImpl::OnUpdate()
{
	bool can_build = true;

	auto item_style = GetStyle()["Item"];

	GLX::Detail::Recycler recycler(m_strings_group, item_style, [this, item_style]() -> TRef <GLX::Object>
	{
		auto row = New<GLX::Object>();

		auto editor = GLX::Init(New<GLX::TextArea>(false), item_style["TextEdit"]);

		GLX::BindEvent(editor, GLX::kTransaction, [this, row, editor](GLX::Object &, GLX::Event & e)
		{
			if (GLX::GetTransactionStage(e) != GLX::kTransactionBegin)
			{
				m_properties.Set(row->id, ToCString(editor->GetText()));

				Update();
			}

			return true;
		});

		GLX::AddInlineFlex(row, editor);

		Data::SetBool(editor, GLX::kWantsFocus, true);

		return row;
	});

	if (auto current = GetCurrentTemplate())
	{
		for (auto & info : current->strings)
		{
			auto value = GetVariable(info.id);

			auto row = recycler.Acquire(info.id);

			auto textarea = Cast<GLX::TextArea>(row->GetFirst());

			auto text = ToWString(value);

			if (textarea->GetText() != text)
			{
				textarea->SetText(ToWString(value));
			}

			GLX::SetText(row, ToWString(info.name));

			GLX::AddInline(m_strings_group, row);

			can_build = can_build && True(Trim<char>(value));
		}

		GLX::SetText(m_template_popup, ToWString(current->name));

		m_info_scroller.SetContent(ParseMarkup(current->description_utf8, GetStyle()["Scroller"]["Markup"]));
	}
	else
	{
		GLX::ClearText(m_template_popup);

		m_info_scroller.SetContent(New<GLX::Object>());

		can_build = false;

		//overlay locate screen
	}

	GLX::Select(m_build_button, can_build);

	GLX::EnableMouse(m_build_button, can_build);
}

//void ViewImpl::SetReflexPath()
//{
//	if (auto path = GLX::ShowFolderDialog(Bootstrap::global->prefs, K32("project_creator.locate"), false, app->GetReflexPath(), L"Locate Reflex"))
//	{
//		app->SetReflexPath(path);
//	}
//}

CString::View ViewImpl::GetVariable(Key32 id) const
{
	auto null = &Reflex::Detail::GetNullInstance<CString>();

	return *m_properties.Search(id, null);
}

TRef <GLX::Object> ViewImpl::ParseMarkup(Data::Archive::View desc, const GLX::Style & style)
{
	REFLEX_LOCAL(TRef <GLX::Object>, Recurse)(const Data::PropertySet & node, const GLX::Style & node_style, const WString::View & text)
	{
		auto object = GLX::Init(New<GLX::Label>(text), node_style);

		GLX::SetFlow(*object, GLX::kFlowY);
		GLX::EnableAutoFit(*object, false, true);

		for (auto & child : Data::GetXmlNodes(node))
		{
			auto tag = Data::GetXmlTag(child);
			auto value = Data::GetCString(child, "value");

			auto child_style = node_style[tag];
			auto child_text = ToWString(value);

			GLX::AddInline(*object, Call(child, child_style, child_text));
		}

		return object;
	}
	REFLEX_END

	auto data = Data::DecodePropertySet(Data::kReflexMarkupFormat, desc);

	return Recurse::Call(data, style, {});
}

} } //end internal namespace

Reflex::TRef <ReflexProjectCreator::View> ReflexProjectCreator::View::Create(App & app)
{
	return Reflex::New<ReflexProjectCreator::ViewImpl>(app);
}

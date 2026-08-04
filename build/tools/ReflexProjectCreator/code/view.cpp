#include "view.h"
#include "../../ReflexCLI/code/tasks.h"




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

	
	CString::View GetVariable(Key32 id) const;

	bool TargetEnabled(CString::View target) const;

	void EnableTarget(CString::View target, bool enabled);


	static WString GetTargetDisplayName(CString::View target);

	static TRef <GLX::Object> ParseMarkup(Data::Archive::View utf8, const GLX::Style & style);


	const TRef <App> app;

	bool m_reset = false;

	WString m_current_template;

	Map <Key32,CString> m_properties;
	Array <CString> m_targets;

	GLX::Object m_header;
	GLX::Popup m_menu_popup;

	enum Sections
	{
		kSectionTemplate,
		kSectionStrings,
		kSectionTargets,

		kNumSection,
	};

	GLX::Object m_sections[kNumSection];
	GLX::Popup m_template_popup;
	GLX::ScrollArea m_info_scroller;
	GLX::Button m_build_button;
};

ViewImpl::ViewImpl(App & app)
	: View(app, 2, L":res:ReflexProjectCreator/styles.glx")
	, app(app)
	, m_build_button(L"Create")
{
	Data::SetBool(*this, GLX::kresizable, true);

	Data::SetBool(m_template_popup, GLX::kfocusable, true);

	GLX::AddFloat(m_header, m_menu_popup, GLX::kAlignmentRight);

	GLX::SetText(m_template_popup, L"Template", "key");

	GLX::AddInline(*this, m_header);

	for (auto & i : m_sections)
	{
		GLX::SetFlow(i, GLX::kFlowY);
		
		GLX::AddInline(*this, i);
	}

	GLX::AddInline(m_sections[kSectionTemplate], m_template_popup);

	GLX::AddInlineFlex(*this, m_info_scroller)->InsertBefore(m_sections[kSectionTargets]);

	GLX::AddInline(*this, m_build_button);

	GLX::EnableTabNavigation(*this);
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
	m_reset = true;
}

void ViewImpl::OnRestoreState(Data::Archive::View & stream, Key32 context)
{
	Data::DeserializeUTF8(stream, m_current_template);

	Data::Deserialize(stream, m_properties, m_targets);
}

void ViewImpl::OnStoreState(Data::Archive & stream) const
{
	Data::SerializeUTF8(stream, m_current_template);

	Data::Serialize(stream, m_properties, m_targets);
}

void ViewImpl::OnSetStyle(const GLX::Style & style)
{
	auto group = style["Group"];

	for (auto & i : m_sections)
	{
		i.SetStyle(group);
	}

	m_header.SetStyle(style["Header"]);
	m_menu_popup.SetStyle(style["Header"]["Popup"]);
	m_template_popup.SetStyle(style["Popup"]);
	
	m_info_scroller.SetStyle(style["Scroller"]);

	m_build_button.SetStyle(style["Button"]);

	m_sections[kSectionTargets].Clear();
	m_sections[kSectionStrings].Clear();

	Update();
}

bool ViewImpl::OnEvent(GLX::Object & source, GLX::Event & e)
{
	auto build = [this](GLX::Event & e)
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

				bool overwrite = True(GLX::GetModifierKeys(e) & GLX::kModifierKeyShift);

				app->InstantiateTemplate(*tmpl, variables, m_targets, dest, overwrite);

				Update();
			}
		}
	};

	if (Bootstrap::View::OnEvent(source, e))
	{
		return true;
	}
	else if (e.id == GLX::kMouseDown)
	{
		if (source == m_build_button)
		{
			build(e);

			return true;
		}
	}
	else if (e.id == GLX::kKeyDown)
	{
		if (GLX::GetKeyCode(e) == GLX::kKeyCodeEnter)
		{
			build(e);

			return true;
		}
	}
	else if (auto menu = GLX::GetMenu(e))
	{
		if (source == m_menu_popup)
		{
			GLX::BindClick(menu->AddItem(L"Refresh Templates"), [this]()
			{
				app->Reset();
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
	output.Log("OnUpdate");

	auto checkbox_style = GetStyle()["Checkbox"];
	auto textedit_style = GetStyle()["TextEdit"];

	GLX::Detail::Recycler targets_recycler(m_sections[kSectionTargets], checkbox_style, [this]() -> TRef <GLX::Object>
	{
		auto item = New<GLX::Button>();

		auto toggle = [this, item]()
		{
			auto target = Data::GetCString(item, "target");

			EnableTarget(target, !TargetEnabled(target));
		};

		GLX::BindClick(item, toggle);

		GLX::BindEvent(item, GLX::kKeyDown, [toggle](GLX::Object &, GLX::Event & e)
		{
			switch (GLX::GetKeyCode(e))
			{
			case GLX::kKeyCodeSpace:
			case GLX::kKeyCodeEnter:
				toggle();
				return true;
			}

			return false;
		});

		Data::SetBool(item, GLX::kfocusable, true);

		return item;
	});

	GLX::Detail::Recycler strings_recycler(m_sections[kSectionStrings], textedit_style, [this]() -> TRef <GLX::Object>
	{
		auto item = New<GLX::TextArea>(false);

		GLX::BindEvent(item, GLX::kTransaction, [this, item](GLX::Object &, GLX::Event & e)
		{
			if (GLX::GetTransactionStage(e) != GLX::kTransactionStageBegin)
			{
				if (SetFiltered(m_properties.Acquire(item->id), ToCString(item->GetText())))
				{
					Update();
				}
			}

			return true;
		});

		Data::SetBool(item, GLX::kfocusable, true);

		return item;
	});

	auto targets = app->GetTargets();

	if (targets)
	{
		if (SetFiltered(m_reset, false))
		{
			m_targets.Clear();

			for (auto [target, enabled] : targets)
			{
				if (enabled) m_targets.Push(target);
			}
		}
	}

	bool can_build = True(m_targets);

	for (auto & [target,enabled] : targets)
	{
		auto item = Cast<GLX::Button>(targets_recycler.Acquire(target));

		Data::SetCString(item, "target", target);

		GLX::SetText(item, GetTargetDisplayName(target));

		GLX::Select(item, TargetEnabled(target));

		GLX::AddInline(m_sections[kSectionTargets], item);
	}

	if (auto current = GetCurrentTemplate())
	{
		for (auto & info : current->strings)
		{
			auto value = GetVariable(info.id);

			auto textarea = Cast<GLX::TextArea>(strings_recycler.Acquire(info.id));

			auto text = ToWString(value);

			textarea->SetText(ToWString(value));

			GLX::SetText(Cast<GLX::Object>(textarea), ToWString(info.name), "key");

			GLX::AddInline(m_sections[kSectionStrings], textarea);

			can_build = can_build && True(Trim(value));
		}

		GLX::SetText(m_template_popup, ToWString(current->name));

		m_info_scroller.SetContent(ParseMarkup(current->description_utf8, GetStyle()["Scroller"]["Markup"]));
	}
	else
	{
		GLX::ClearText(m_template_popup);

		m_info_scroller.SetContent(New<GLX::Object>());

		can_build = false;
	}


	Data::SetBool(m_build_button, GLX::kfocusable, can_build);

	GLX::Activate(m_build_button, can_build);
}

CString::View ViewImpl::GetVariable(Key32 id) const
{
	auto null = &Reflex::Detail::GetNullInstance<CString>();

	return *m_properties.Search(id, null);
}

bool ViewImpl::TargetEnabled(CString::View target) const
{
	return True(Search(m_targets, target));
}

void ViewImpl::EnableTarget(CString::View target, bool enabled)
{
	Remove(m_targets, target);

	if (enabled)
	{
		m_targets.Push(target);
	}

	Update();
}

WString ViewImpl::GetTargetDisplayName(CString::View target)
{
	constexpr Pair <CString::View> kTargets[] =
	{
		{ "visual_studio", "Visual Studio" },
		{ "android_studio", "Android Studio" },
		{ "xcode", "Xcode" },
		{ "cmake", "CMake" }
	};

	if (auto match = SearchValue<KeyCompare>(ToView(kTargets), target))
	{
		return ToWString(match->b);
	}

	return ToWString(target);
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

#include "console.h"
#include "../globalimpl.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::IDE)

struct Console : public Object
{
	typedef The <Console> TheConsole;

	struct WindowDelegate : public GLX::WindowClient
	{
		REFLEX_OBJECT(WindowDelegate, GLX::WindowClient);

		using GLX::WindowClient::WindowClient;
	};

	struct MainWindowDelegate : public WindowDelegate
	{
		MainWindowDelegate(Console & console, System::Window & owner, AlreadyRetained <GLX::Object> root, const Function <void()> & onclose);

		~MainWindowDelegate();

		virtual void OnClose() override
		{
			m_onclose();
		}

		AlreadyRetained <Console> console;

		const GLX::StyleSheet & styles;

		Array < Tuple <WString,Detail::ConsolePanel&> > m_panels;

		Function <void()> m_onclose;

		GLX::TabGroup m_tabgroup;

		Key32 m_focused;
	};

	struct UndockedPanel : public Object
	{
		UndockedPanel(MainWindowDelegate & delegate, Key32 panelid);

		~UndockedPanel();

		Reference <System::Window> m_window;

		Reference <WindowDelegate> m_glxwindow;

		Reference <GLX::Object> m_button;
	};

	Console(AlreadyRetained <GLX::Object> root, const Function <void()> & onclose)
		: m_global(TheGlobal::Get()),
		m_window(System::Window::Create(System::kWindowStyleResizable | System::kWindowStyleMinimisable, Data::GetBool(m_global->m_prefs, kOnTop))),
		windowdlg(New<MainWindowDelegate>(*this, m_window, root, onclose))
	{
	}

	~Console();


	Reference <GlobalImpl> m_global;

	Reference <System::Window> m_window;

	Key32 m_undocked;

	MainWindowDelegate & windowdlg;


	static constexpr Key32 kPrefsKey = K32("ide.console.state");

	static constexpr Key32 kOnTop = K32("ide.console.ontop");

	static constexpr Key32 kUndockedRect = K32("ide.console.undocked");
};

Console::~Console()
{
	if (GLX::module.IsInitialised())
	{
		GLX::Core::Context ctx;

		m_window.Clear();
	}
}

Console::MainWindowDelegate::MainWindowDelegate(Console & console, System::Window & owner, AlreadyRetained <GLX::Object> root, const Function <void()> & onclose)
	: WindowDelegate(false),
	console(console),
	styles(Detail::RetrieveStyleSheet()),
	m_panels(Detail::CreatePanels(root)),
	m_onclose(onclose)
{
	Retain(styles);

	m_tabgroup.SetStyle(styles["Dialog"]);


	//add panels sorted

	for (auto & i : m_panels)
	{
		m_tabgroup.AddPanel(i.a, i.b, GLX::kcontent, i.a);
	}

	Data::SetBool(m_tabgroup, GLX::kresizable, true);

	SetContent(m_tabgroup);



	//restore data

	constexpr auto DefaultRect = []()
	{
		const auto screen = GLX::Detail::ToFloat(System::GetScreens()[0]);

		GLX::Rect rect = { { screen.origin }, { 448.0f, screen.size.h - 96.0f } };

		rect.origin += GLX::Detail::Align(screen.size, rect.size, GLX::kAlignmentLeft);

		return rect;
	};

	GLX::Rect rect;

	m_focused = m_panels.GetFirst().b.id;

	//auto selector = m_tabgroup.GetSelector();

	Data::PropertySet propertyset;

	if (auto stream = Data::GetBinary(TheGlobal::Get()->m_prefs, kPrefsKey))
	{
		Data::Deserialize(stream, rect, Reinterpret<UInt32>(m_focused), Reinterpret<UInt32>(console.m_undocked));

		bool ok = false;

		for (auto & irect : System::GetScreens())
		{
			ok = ok || GLX::Contains(GLX::Detail::ToFloat(irect), rect);
		}

		if (!ok) rect = DefaultRect();

		Data::kBinaryFormat->Deserialize(stream, propertyset);
	}
	else
	{
		rect = DefaultRect();
	}



	//display

	REFLEX_LOOP(idx, m_panels.GetSize())
	{
		auto & panel = m_panels[idx].b;

		Key32 panelid = panel.id;

		Detail::RestoreSerializable(propertyset, {}, panel);

		if (panelid == m_focused) m_tabgroup.GetSelector()->SelectPanel(idx);

		if (panelid == console.m_undocked)
		{
			m_tabgroup.SetProperty(kNullKey, REFLEX_CREATE(UndockedPanel, *this, panelid));
		}
	}

	rect.size = Max(rect.size, GLX::Detail::ComputeContentSize(m_tabgroup));

	Pair <System::WindowDisplay,System::iRect> unused;
	owner.SetClient(this, unused.a, unused.b);
	owner.SetRect({ { Truncate(rect.origin.x), Truncate(rect.origin.y) }, { Truncate(rect.size.w), Truncate(rect.size.h) } });
	owner.SetDisplayMode(System::kWindowDisplayWindowed);
	SetTitle(L"Reflex");


	GLX::BindEvent(m_tabgroup, GLX::Selector::kSelectPanel, [this](GLX::Object & src, GLX::Event & e)
	{
		if (src == m_tabgroup)
		{
			auto & delegate = TheConsole::Get()->windowdlg;

			delegate.m_focused = GLX::GetItem(e)->id;
		}

		return false;
	});

	GLX::BindEvent(m_tabgroup.header, GLX::kMouseDown, [this](GLX::Object & src, GLX::Event & e)
	{
		if (GLX::IsRightClick(e))
		{
			for (auto & i : m_panels)
			{
				if (GLX::GetText(src) == i.a)
				{
					auto menu = GLX::OpenContextMenu(GetContent());

					Key32 panelid = i.a;

					GLX::BindClick(menu->AddItem(L"Undock"), [this, panelid]()
					{
						m_tabgroup.SetProperty(kNullKey, REFLEX_CREATE(UndockedPanel, *this, panelid));
					});

					return true;
				}
			}

			auto menu = GLX::OpenContextMenu(GetContent());

			const Tuple <Key32,WString::View> options[] = { {kOnTop, L"Always On Top"} };

			auto & prefs = *TheGlobal::Get()->m_prefs;

			for (auto & i : ToView(options))
			{
				auto id = i.a;

				bool value = Data::GetBool(prefs, id);

				GLX::BindClick(GLX::AddMenuOption(menu, i.b, value), [&prefs, id, value]()
				{
					Data::SetBool(prefs, id, !value);

					GLX::Restart();
				});
			}

			return true;
		}

		return false;
	});
}

Console::MainWindowDelegate::~MainWindowDelegate()
{
	auto archive = New<Data::ArchiveObject>();

	Data::Serialize(archive->value, GetRect(), m_focused, console->m_undocked);

	Data::PropertySet propertyset;

	for (auto & i : m_panels)
	{
		Detail::StoreSerializable(propertyset, i.b);
	}

	Data::kBinaryFormat->Serialize(archive->value, propertyset);

	TheGlobal::Get()->m_prefs->SetProperty(kPrefsKey, archive);

	m_tabgroup.Clear();

	Release(styles);
}

Console::UndockedPanel::UndockedPanel(MainWindowDelegate & windowdlg, Key32 panelid)
	: m_window(System::Window::Create(System::kWindowStyleResizable | System::kWindowStyleMinimisable, true)),
	m_glxwindow(New<WindowDelegate>(false))
{
	auto & tabgroup = windowdlg.m_tabgroup;

	auto selector = tabgroup.GetSelector();
	GLX::Rect rect;

	REFLEX_LOOP(idx, selector->GetNumPanel())
	{
		auto panel = selector->GetPanel(idx);

		if (panelid == panel->id)
		{
			Data::SetBool(panel, GLX::kresizable, true);	//stop stylsheet refresh from doing AutoFit

			windowdlg.console->m_undocked = panelid;

			m_button = GLX::LookupChildAtIndex(windowdlg.m_tabgroup.header, idx);

			GLX::Detail::Hide(m_button);

			rect = { { 64.0f, 64.0f }, selector->GetRect().size };

			if (auto binary = Data::GetBinary(TheGlobal::Get()->m_prefs, kUndockedRect))
			{
				rect = Data::Unpack<GLX::Rect>(binary);
			}

			m_glxwindow->SetContent(panel);

			m_glxwindow->SetBackgroundColour(windowdlg.GetBackgroundColour());

			auto contentsize = GLX::Detail::ComputeContentSize(panel);

			rect.size = Max(rect.size, contentsize);

			GLX::BindEventVoid(panel, GLX::kRequestClose, []()
			{
				auto console = TheConsole::Get();

				console->m_undocked = Key32();

				console->windowdlg.m_tabgroup.UnsetProperty<UndockedPanel>(kNullKey);
			});
		}
	}

	Pair <System::WindowDisplay,System::iRect> unused;
	m_window->SetClient(m_glxwindow, unused.a, unused.b);
	m_window->SetRect({ { Truncate(rect.origin.x), Truncate(rect.origin.y) }, { Truncate(rect.size.w), Truncate(rect.size.h) } });
	m_window->SetDisplayMode(System::kWindowDisplayWindowed);
}

Console::UndockedPanel::~UndockedPanel()
{
	Data::SetBinary(TheGlobal::Get()->m_prefs, kUndockedRect, Data::Pack(m_glxwindow->GetRect()));

	GLX::Detail::Show(*m_button);
}

REFLEX_END_INTERNAL

Reflex::Unretained <Reflex::Object> Reflex::IDE::AcquireConsole(AlreadyRetained <GLX::Object> root, const Function <void()> & onclose)
{
	REFLEX_ASSERT(IDE::kIsAwake && GLX::module.IsInitialised());

	GLX::Core::Context ctx;

	GLX::AnimationScope animate(false);

	return Unretained<Object>(*Reflex::The<Console>::Acquire(root, onclose));
}

Reflex::IDE::ComputedStyle::ComputedStyle(const GLX::Style & style)
	: menu(style.QuerySubStyle("menu", &Reflex::Detail::GetNullInstance<GLX::Style>()))
	, menu_section(menu->QuerySubStyle("section", &Reflex::Detail::GetNullInstance<GLX::Style>()))
	, bar(style.QuerySubStyle("Bar", &Reflex::Detail::GetNullInstance<GLX::Style>()))
	, subgroup(bar->QuerySubStyle("SubGroup", &Reflex::Detail::GetNullInstance<GLX::Style>()))
	, button(style.QuerySubStyle("Button", &Reflex::Detail::GetNullInstance<GLX::Style>()))
	, popup(style.QuerySubStyle("Popup", &Reflex::Detail::GetNullInstance<GLX::Style>()))
	, list(style.QuerySubStyle("List", &Reflex::Detail::GetNullInstance<GLX::Style>()))
	, list_item(style.QuerySubStyle("InfoItem", &Reflex::Detail::GetNullInstance<GLX::Style>()))
	, texteditor(style.QuerySubStyle("TextEditor", &Reflex::Detail::GetNullInstance<GLX::Style>()))
	, focus_rectangle(style.QuerySubStyle("Rectangle", &Reflex::Detail::GetNullInstance<GLX::Style>()))
	, focus_parent_rectangle(style.QuerySubStyle("ParentRectangle", &Reflex::Detail::GetNullInstance<GLX::Style>()))
{
}

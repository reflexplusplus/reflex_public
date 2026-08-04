#include "[require].h"
#include "settingsdialog.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap)

template <class TYPE> inline TYPE GetConfig(Key32 id, TYPE fallback = {})
{
	return UnpackResource<TYPE>(K32("Config"), id, fallback);
}

struct WindowClient : public GLX::WindowClient
{
	WindowClient(System::Window & owner, bool ide_enabled)
		: GLX::WindowClient(ide_enabled)
	{
		owner.SetClient(this);
	}

	void SetContent(GLX::Object & view)
	{
		if (profiler && GetConfig<bool>(K32("view.allow_ide"), true))
		{
			m_console = IDE::AcquireConsole(view, []()
			{
				global->EnableIde(false);
			});
		}

		GLX::WindowClient::SetContent(view);
	}

	Reference <Reflex::Object> m_console;
};

struct PluginWindowClient : 
	public WindowClient, 
	public Streamable
{
	PluginWindowClient(System::Window & owner, bool ide_enabled, File::PersistentPropertySet & session, GLX::Object & view)
		: WindowClient(owner, ide_enabled)
		, Streamable(session, K32("bootstrap.plugin_window_size"), 1)
		, m_resizable(Data::GetBool(view, GLX::kresize))
	{
		SetContent(view);

		Streamable::RestoreState();

		SetDisplayMode(System::kWindowDisplayWindowed);	//TODO this should not be neccesary on plugin
	}

	~PluginWindowClient()
	{
		Streamable::StoreState();
	}

	void OnReset(Key32 context) override
	{
		SetRect({ {}, GetContent()->contentsize });
	}

	void OnRestore(Data::Archive::View & stream, Key32 context) override
	{
		auto content_size = GetContent()->contentsize;

		if (m_resizable)
		{
			SetRect({ {}, Max(content_size, Data::Deserialize<System::fSize>(stream))});
		}
		else
		{
			SetRect({ {}, content_size });
		}
	}

	void OnStore(Data::Archive & stream) const override
	{
		Data::Serialize(stream, GetRect().size);
	}

	bool m_resizable;
};

struct DesktopAppWindowClient : public WindowClient
{
	static constexpr UInt32 kWindowState = K32("bootstrap.window_state");

	DesktopAppWindowClient(System::Window & owner, bool ide_enabled, GLX::Object & view)
		: WindowClient(owner, ide_enabled)
	{
		SetContent(view);

		GLX::Rect default_rect = { { 64.0f, 64.0f }, view.contentsize };

		auto restored_mode = System::kWindowDisplayWindowed;

		auto restored_rect = default_rect;

		if (auto binary = Data::GetBinary(global->prefs, kWindowState))
		{
			restored_mode = Max(System::WindowDisplay(binary[0]), System::kWindowDisplayWindowed);

			if (Data::GetBool(view, GLX::kresize))
			{
				Data::Unpack(Mid(binary, 1, 16), restored_rect);

				restored_rect.size = Max(restored_rect.size, default_rect.size);
			}
			else
			{
				Data::Unpack(Mid(binary, 1, 8), restored_rect.origin);
			}

			restored_rect = Detail::ConstrainRectToDisplay(restored_rect, default_rect.size);
		}

		SetRect(restored_rect);

		SetDisplayMode(restored_mode);
	}

	~DesktopAppWindowClient()
	{
		REFLEX_STATIC_ASSERT(kSizeOf<GLX::Rect> == 16);

		UInt8 window_state[17];
		window_state[0] = UInt8(GetDisplayMode());
		MemCopy(&GetRect(), window_state + 1, 16);

		Data::SetBinary(global->prefs, kWindowState, ToView(window_state));
	}

	void OnClose() override
	{
		System::App::Quit();
	}
};

struct MobileWindowClient : public WindowClient
{
	static constexpr UInt32 kPanels = K32("bootstrap.mobile_view");

	MobileWindowClient(System::Window & owner, bool ide_enabled, GLX::Object & view)
		: WindowClient(owner, ide_enabled)
	{
		if (ide_enabled)
		{
			auto styles = IDE::Detail::RetrieveStyleSheet();

			auto tabgroup = Make<GLX::TabGroup>();

			tabgroup->SetStyle(styles["Dialog"]);

			GLX::SetFlow(tabgroup->header, GLX::kFlowCenter);

			GLX::SetClip(view, K32("Bootstrap"));

			tabgroup->AddPanel(L"View", view, GLX::kcontent, K32("UI"));

			Data::PropertySet propertyset;

			if (auto chunk = Data::GetBinary(global->prefs, kPanels))
			{
				Data::kBinaryFormat->Deserialize(chunk, propertyset);
			}

			for (auto & i : IDE::Detail::CreatePanels(view))
			{
				IDE::Detail::RestoreStreamable(propertyset, {}, i.b);

				tabgroup->AddPanel(i.a, i.b, GLX::kcontent, i.a);
			}

			tabgroup->GetSelector()->SelectPanel(0);

			auto close = GLX::AddFloat(tabgroup->header, GLX::Init(New<GLX::Button>(), styles["Close"]), GLX::kAlignmentRight);

			GLX::BindClick(close, []()
			{
				global->EnableIde(false);
			});

			SetContent(tabgroup);
		}
		else
		{
			SetContent(view);
		}
	}

	~MobileWindowClient()
	{
		if (auto tabgroup = DynamicCast<GLX::TabGroup>(GetContent()))
		{
			Data::Archive stream;

			Data::PropertySet propertyset;

			auto selector = tabgroup->GetSelector();

			REFLEX_LOOP(idx, selector->GetNumPanel())
			{
				IDE::Detail::StoreStreamable(propertyset, selector->GetPanel(idx));
			}

			Data::kBinaryFormat->Serialize(stream, propertyset);

			Data::SetBinary(global->prefs, kPanels, stream);
		}

		global->CommitPreferences();
	}

	System::ScreenOrientation OnGetScreenOrientation() override
	{
		switch (GetConfig<Key32>(K32("view.screen_orientation"), {}).value)
		{
		case K32("portrait"):
			return System::kScreenOrientationPortrait;

		case K32("landscape"):
			return System::kScreenOrientationLandscape;

		default:
			return System::kScreenOrientationDefault;
		}
	}

	void OnSetRect(System::WindowDisplay state, const System::iRect & rect, const System::iRect & interactable, Int32 dpifactor) override
	{
		auto style = Make<GLX::Style>(kNullKey);

		auto padding = GLX::MakeMargin(ToFloat32(interactable.origin.x), ToFloat32(interactable.origin.y), ToFloat32(rect.size.w - interactable.size.w), ToFloat32(rect.size.h - interactable.size.h));

		GLX::SetMargin(style, K32("padding"), padding);

		GetContent()->SetMod(K32("client_area"), GLX::Detail::ComputedStyle::Create(style));

		GLX::WindowClient::OnSetRect(state, rect, interactable, dpifactor);
	}

	void OnClose() override
	{
		System::App::Quit();
	}
};

REFLEX_END_INTERNAL

Reflex::Reference <Reflex::GLX::WindowClient> Reflex::Bootstrap::Detail::CreateWindowClient(System::Window & window, GLX::Object & content, File::PersistentPropertySet & session)
{
	bool ide_enabled = global->IdeEnabled();

	switch (System::kEnvironmentType)
	{
	case System::kEnvironmentTypeAudioPlugin:
	case System::kEnvironmentTypeLibrary:
		return New<PluginWindowClient>(window, ide_enabled, session, content);

	case System::kEnvironmentTypeDesktopApp:
		if (!GLX::kIsMobile)
		{
			return New<DesktopAppWindowClient>(window, ide_enabled, content);
		}

	default:
		return New<MobileWindowClient>(window, ide_enabled, content);
	}
}

void Reflex::Bootstrap::Detail::PublishAppView(System::App::Configuration & config, const Function <TRef<GLX::Object>(Object & instance_delegate)> & ctr)
{
	config.view_ctr = [ctr](System::App & system, void * host_window)
	{
		auto config = AcquireProperty<Data::MapOfKey32Property<UInt32>>(global->prefs, kViewGraphicsConfig);	//restore graphics settings

		auto glx = GLX::Start(global->resourcepool, config->value);	//start GLX, the window will retain the object, so we dont need to store

		GLX::Core::Context ctx;

		GLX::AnimationScope scope(GetConfig<bool>(K32("view.allow_init_animations"), true));

		auto app = system.GetClient();

		auto session = app->QueryProperty<File::PersistentPropertySet>("session", Bootstrap::global->prefs.Adr());	//need this backdoor temporarily, to support legacy non-App clients

		auto view = ctr(app);

		auto resizeable = Data::GetBool(view, GLX::kresize);

		auto window = System::Window::Create(System::kWindowStyleFrame | System::kWindowStyleMinimisable | (resizeable ? System::kWindowStyleResizable : 0), false, host_window);

		auto client = Detail::CreateWindowClient(*window, view, *session);

		client->SetTitle(ToWString(global->product));

		GLX::FocusBranch(view);

		return window;
	};
}

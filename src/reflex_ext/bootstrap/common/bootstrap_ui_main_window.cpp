#include "[require].h"
#include "settingsdialog.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap)

template <class TYPE> inline TYPE GetConfig(Key32 id, TYPE fallback = {})
{
	return File::UnpackResource<TYPE>("Config", id, fallback);
}

struct WindowClient : public GLX::WindowClient
{
	WindowClient()
		: GLX::WindowClient(global->IdeEnabled())
	{
	}

	void OnAttachWindow(System::Window & window, System::WindowDisplay & initial_mode, System::iRect & initial_rect) override
	{
		GLX::WindowClient::OnAttachWindow(window, initial_mode, initial_rect);

		window.SetTitle(ToWString(global->product));
	}

	void SetContent(GLX::Object & view)
	{
		if (profiler && GetConfig<bool>("view.allow_ide", true))
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
	public PersistentState
{
	PluginWindowClient(File::PersistentPropertySet & session, GLX::Object & view)
		: WindowClient()
		, PersistentState(session, "bootstrap.plugin_window_size", 1)
		, m_resizable(Data::GetBool(view, GLX::kresizable))
	{
		SetContent(view);
		PersistentState::RestoreState();
	}

	~PluginWindowClient()
	{
		PersistentState::StoreState();
	}

	void OnReset(Key32 context) override
	{
		SetSize(GetContent()->contentsize);
	}

	void OnRestore(Data::Archive::View & stream, Key32 context) override
	{
		auto content_size = GetContent()->contentsize;

		if (m_resizable)
		{
			SetSize(Max(content_size, Data::Deserialize<System::fSize>(stream)));
		}
		else
		{
			SetSize(content_size);
		}
	}

	void OnStore(Data::Archive & stream) const override
	{
		Data::Serialize(stream, m_last_size);
	}

	void OnAttachWindow(System::Window & window, System::WindowDisplay & initial_mode, System::iRect & initial_rect) override
	{
		WindowClient::OnAttachWindow(window, initial_mode, initial_rect);

		initial_rect.size = { Truncate(m_last_size.w), Truncate(m_last_size.h) };
	}

	void OnSetRect(System::WindowDisplay mode, const System::iRect & rect, const System::iRect & client_area, Int32 dpifactor) override
	{
		WindowClient::OnSetRect(mode, rect, client_area, dpifactor);

		if (mode == System::kWindowDisplayWindowed) m_last_size = GetRect().size;
	}

	void SetSize(GLX::Size size)
	{
		m_last_size = size;

		owner->SetRect({ {}, { Truncate(size.w), Truncate(size.h) } });
	}


	bool m_resizable;

	GLX::Size m_last_size;
};

struct DesktopAppWindowClient : public WindowClient
{
	static constexpr UInt32 kWindowState = K32("bootstrap.window_state");

	DesktopAppWindowClient(GLX::Object & view)
		: WindowClient()
	{
		SetContent(view);

		GLX::Rect default_rect = { { 64.0f, 64.0f }, view.contentsize };

		auto restored_mode = System::kWindowDisplayWindowed;

		auto restored_rect = default_rect;

		if (auto binary = Data::GetBinary(global->prefs, kWindowState))
		{
			restored_mode = Max(System::WindowDisplay(binary[0]), System::kWindowDisplayWindowed);

			if (Data::GetBool(view, GLX::kresizable))
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

		m_pending_rect = { { Truncate(restored_rect.origin.x), Truncate(restored_rect.origin.y) }, { Truncate(restored_rect.size.w), Truncate(restored_rect.size.h) } };
		m_windowed_rect = restored_rect;
		m_initial_mode = restored_mode;
	}

	~DesktopAppWindowClient()
	{
		REFLEX_STATIC_ASSERT(kSizeOf<GLX::Rect> == 16);

		UInt8 window_state[17];
		window_state[0] = UInt8(GetDisplayMode());
		MemCopy(&m_windowed_rect, window_state + 1, 16);

		Data::SetBinary(global->prefs, kWindowState, ToView(window_state));
	}

	void OnClose() override
	{
		System::App::Quit();
	}

	void OnAttachWindow(System::Window & window, System::WindowDisplay & initial_mode, System::iRect & initial_rect) override
	{
		WindowClient::OnAttachWindow(window, initial_mode, initial_rect);

		initial_mode = m_initial_mode;
		initial_rect = m_pending_rect;
	}

	void OnSetRect(System::WindowDisplay mode, const System::iRect & rect, const System::iRect & client_area, Int32 dpifactor) override
	{
		WindowClient::OnSetRect(mode, rect, client_area, dpifactor);

		if (mode == System::kWindowDisplayWindowed) m_windowed_rect = GetRect();
	}

	System::iRect m_pending_rect;
	GLX::Rect m_windowed_rect;
	System::WindowDisplay m_initial_mode;
};

struct MobileWindowClient : public WindowClient
{
	static constexpr UInt32 kMobileView = K32("bootstrap.mobile_view");

	static System::ScreenOrientation GetScreenOrientation()
	{
		switch (GetConfig<Key32>("view.screen_orientation", {}).value)
		{
		case MakeKey32("portrait"):
			return System::kScreenOrientationPortrait;

		case MakeKey32("landscape"):
			return System::kScreenOrientationLandscape;

		default:
			return System::kScreenOrientationDefault;
		}
	}

	static void ApplyClientAreaInset(GLX::Object & container, GLX::Object & content, const GLX::Margin & padding)
	{
		auto style = Make<GLX::Style>(kNullKey);

		GLX::SetMargin(style, K32("padding"), padding);

		container.SetMod(K32("client_inset"), GLX::Detail::ComputedStyle::Create(style));

		GLX::SetMargin(content, K32("client_inset"), padding);
	}

	MobileWindowClient(GLX::Object & view)
		: WindowClient()
		, m_view(view)
	{
		if (global->IdeEnabled())
		{
			auto styles = IDE::Detail::RetrieveStyleSheet();

			auto tabgroup = Make<GLX::TabGroup>();

			tabgroup->SetStyle(styles["Dialog"]);

			GLX::SetFlow(tabgroup->header, GLX::kFlowCenter);

			GLX::SetClip(view, kMobileView);

			tabgroup->AddPanel(L"View", view, GLX::kcontent, "UI");

			Data::PropertySet propertyset;

			if (auto chunk = Data::GetBinary(global->prefs, kMobileView))
			{
				Data::kBinaryFormat->Deserialize(chunk, propertyset);
			}

			for (auto & i : IDE::Detail::CreatePanels(view))
			{
				IDE::Detail::RestoreSerializable(propertyset, {}, i.b);

				tabgroup->AddPanel(i.a, i.b, GLX::kcontent, i.a);
			}

			tabgroup->GetSelector()->SelectPanel(0);

			auto close = GLX::AddFloat(tabgroup->header, GLX::Init(New<GLX::Button>(), styles["Close"]), GLX::kAlignmentRight);

			GLX::BindClick(close, []()
			{
				global->EnableIde(false);
			});

			GLX::WindowClient::SetContent(tabgroup);
		}
		else
		{
			GLX::WindowClient::SetContent(view);
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
				IDE::Detail::StoreSerializable(propertyset, selector->GetPanel(idx));
			}

			Data::kBinaryFormat->Serialize(stream, propertyset);

			Data::SetBinary(global->prefs, kMobileView, stream);
		}

		global->CommitPreferences();
	}

	System::ScreenOrientation OnGetScreenOrientation() override
	{
		return GetScreenOrientation();
	}

	void OnSetRect(System::WindowDisplay state, const System::iRect & rect, const System::iRect & client_area, Int32 dpifactor) override
	{
		auto content = GetContent();
		
		auto padding = GLX::MakeMargin(ToFloat32(client_area.origin.x), ToFloat32(client_area.origin.y), ToFloat32(rect.size.w - client_area.size.w), ToFloat32(rect.size.h - client_area.size.h));

		ApplyClientAreaInset(content, content, padding);

		GLX::WindowClient::OnSetRect(state, rect, client_area, dpifactor);
	}

	AlreadyRetained <GLX::Object> m_view;
};

struct EmulatedMobileWrapper : public GLX::Object
{
	struct Device
	{
		CString::View name;
		System::iSize size;
		GLX::Margin portrait_insets;
		GLX::Margin landscape_insets;
	};

	static const Device & GetEmulatedDevice()
	{
		return kDevices[GetEmulatedDeviceIndex()];
	}

	static bool IsLandscape()
	{
		return Data::GetBool(global->prefs, kLandscape, MobileWindowClient::GetScreenOrientation() == System::kScreenOrientationLandscape);
	}


	EmulatedMobileWrapper(GLX::Object & view)
		: m_view(view)
		, m_landscape(L"LS")
		, m_magnify_out(L"-")
		, m_magnify_in(L"+")
	{
		auto styles = IDE::Detail::RetrieveStyleSheet();

		m_header.SetStyle(styles["Bar"]);

		m_popup.SetStyle(styles["Popup"]);

		auto button = styles["Button"];

		m_magnify_out.SetStyle(button);

		m_magnify_in.SetStyle(button);

		m_landscape.SetStyle(button);


		GLX::SetFlow(*this, GLX::kFlowY);

		GLX::SetClip(view, {});

		GLX::AddInline(m_header, m_landscape);
		GLX::AddInline(m_header, m_magnify_out);
		GLX::AddInline(m_header, m_magnify_in);

		GLX::AddInlineFlex(m_header, m_popup);

		GLX::AddInline(*this, m_header);

		SetMagnification(GetMagnification());

		GLX::AddInline(*this, view);

		EnableOnAttachDetachWindow();
	}



private:

	static UInt GetEmulatedDeviceIndex()
	{
		return Min<UInt>(Data::GetUInt32(global->prefs, kMobileDevice, 0), GetArraySize(kDevices) - 1);
	}

	static Float32 GetMagnification()
	{
		return Data::GetFloat32(global->prefs, kMagnification, 1.0f);
	}

	void OnAttachWindow() override
	{
		SelectDevice(GetEmulatedDevice(), IsLandscape());
	}

	bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (auto menu = GLX::GetMenu(e); menu && src == m_popup)
		{
			auto current_idx = GetEmulatedDeviceIndex();

			UInt idx = 0;

			for (const auto & device : kDevices)
			{
				GLX::AddMenuOption(menu, ToWString(device.name), idx++ == current_idx);
			}

			GLX::BindMenuSelect(menu, [this](UInt index)
			{
				Data::SetUInt32(global->prefs, kMobileDevice, index);

				SelectDevice(kDevices[index], IsLandscape());

				RefreshView();
			});

			return true;
		}
		else if (e.id == GLX::kMouseDown)
		{
			static constexpr auto kMagnificationStep = 1.0f / 8.0f;

			if (src == m_magnify_out)
			{
				SetMagnification(GetMagnification() - kMagnificationStep);

				return true;
			}
			else if (src == m_magnify_in)
			{
				SetMagnification(GetMagnification() + kMagnificationStep);

				return true;
			}
			else if (src == m_landscape)
			{
				auto landscape = !IsLandscape();

				Data::SetBool(global->prefs, kLandscape, landscape);

				SelectDevice(GetEmulatedDevice(), landscape);

				RefreshView();

				return true;
			}
		}

		return GLX::Object::OnEvent(src, e);
	}

	void SelectDevice(const Device & device, bool landscape)
	{
		GLX::SetText(m_popup, ToWString(device.name));
		GLX::SetText(m_popup, Join(ToWString(device.size.w), L'x', ToWString(device.size.h)), "info");
		GLX::Select(m_landscape, landscape);

		auto fsize = MakeSize(ToFloat32(device.size.w), ToFloat32(device.size.h));

		if (landscape) Swap(fsize.w, fsize.h);

		GLX::SetBounds(m_view, "mobile_emulation", fsize, fsize);

		MobileWindowClient::ApplyClientAreaInset(m_view, GetWindow()->GetContent(), landscape ? device.landscape_insets : device.portrait_insets);
	}

	void SetMagnification(Float magnification)
	{
		magnification = Clip(magnification, 0.5f, 1.0f);

		bool magnified = magnification < 1.0f;

		if (magnified)
		{
			m_view->SetMod(kMagnification, GLX::Detail::ComputedStyle::Create(magnification, 1.0f, GLX::Detail::ComputedStyle::kRenderAuto));
		}
		else
		{
			m_view->UnsetMod(kMagnification);
		}

		Data::SetFloat32(global->prefs, kMagnification, magnification);

		GLX::Activate(m_magnify_out, magnification > 0.5f);
		GLX::Activate(m_magnify_in, magnified);

		GetWindow()->AutoFit();
	}

	void RefreshView()
	{
		//discard stylesheets from resource pool, to force full-reload with conditionals re-applied

		File::ResourcePool::Lock lock(Bootstrap::global->resourcepool);

		lock.Clear(GetTypeID<GLX::StyleSheet>());

		GetWindow()->AutoFit();
	}



	AlreadyRetained <GLX::Object> m_view;

	GLX::Button m_landscape, m_magnify_out, m_magnify_in;

	GLX::Popup m_popup;

	GLX::Object m_header;


	static constexpr Key32 kMobileDevice = "bootstrap.mobile_device";
	static constexpr Key32 kMagnification = "bootstrap.mobile_magnification";
	static constexpr Key32 kLandscape = "bootstrap.mobile_landscape";

	// Full display size in logical units. Android insets assume gesture navigation on the Pixel 8, A25 and A51.
	static constexpr Device kDevices[] =
	{
		{ "Samsung Galaxy S7", { 360, 640 }, GLX::MakeMargin(0, 24, 0, 0), GLX::MakeMargin(0, 24, 0, 0) },
		{ "iPhone SE", { 375, 667 }, GLX::MakeMargin(0, 20, 0, 0), GLX::MakeMargin(0, 0, 0, 0) },
		{ "iPhone 13 mini", { 375, 812 }, GLX::MakeMargin(0, 50, 0, 34), GLX::MakeMargin(50, 0, 50, 21) },
		{ "Pixel 8", { 360, 800 }, GLX::MakeMargin(0, 32, 0, 24), GLX::MakeMargin(32, 24, 0, 24) },
		{ "iPhone 13", { 390, 844 }, GLX::MakeMargin(0, 47, 0, 34), GLX::MakeMargin(47, 0, 47, 21) },
		{ "iPhone 15 / Pro", { 393, 852 }, GLX::MakeMargin(0, 59, 0, 34), GLX::MakeMargin(59, 0, 59, 21) },
		{ "iPhone 17", { 402, 874 }, GLX::MakeMargin(0, 62, 0, 34), GLX::MakeMargin(62, 20, 62, 20) },
		{ "iPhone 15 Plus / Pro Max", { 430, 932 }, GLX::MakeMargin(0, 59, 0, 34), GLX::MakeMargin(59, 0, 59, 21) },
		{ "iPhone 17 Pro Max", { 440, 956 }, GLX::MakeMargin(0, 62, 0, 34), GLX::MakeMargin(62, 20, 62, 20) },
		{ "Samsung Galaxy A25", { 360, 780 }, GLX::MakeMargin(0, 32, 0, 24), GLX::MakeMargin(32, 24, 0, 24) },
		{ "Samsung Galaxy S8+", { 360, 740 }, GLX::MakeMargin(0, 24, 0, 48), GLX::MakeMargin(0, 24, 48, 0) },
		{ "Samsung Galaxy A51", { 360, 800 }, GLX::MakeMargin(0, 32, 0, 24), GLX::MakeMargin(32, 24, 0, 24) },
		{ "iPad Mini", { 744, 1133 }, GLX::MakeMargin(0, 24, 0, 20), GLX::MakeMargin(0, 24, 0, 20) },
		{ "iPad", { 820, 1180 }, GLX::MakeMargin(0, 24, 0, 20), GLX::MakeMargin(0, 24, 0, 20) },
	};
};

REFLEX_END_INTERNAL

Reflex::Unretained <Reflex::GLX::WindowClient> Reflex::Bootstrap::Detail::CreateAppWindow(System::Window & window, GLX::Object & view)
{
	auto window_client = New<WindowClient>();
	window_client->SetContent(view);
	Pair <System::WindowDisplay,System::iRect> unused;
	window.SetClient(window_client, unused.a, unused.b);
	return window_client;
}

void Reflex::Bootstrap::Detail::PublishAppView(System::App::Configuration & config, const Function <Unretained<GLX::Object>(Object & instance_delegate)> & ctr)
{
	config.view_ctr = [ctr](System::App & system, UInt8 & window_flags) -> Unretained<System::Window::Client>
	{
		auto config = AcquireProperty<Data::MapOfKey32Property<UInt32>>(global->prefs, kViewGraphicsConfig);	//restore graphics settings

		auto glx = GLX::Start(global->resourcepool, config->value);	//start GLX, the window will retain the object, so we dont need to store

		auto app = system.GetClient();

		auto session = app->QueryProperty<File::PersistentPropertySet>("session", Bootstrap::global->prefs.Adr());	//need this backdoor temporarily, to support legacy non-App clients

		GLX::Core::Context ctx;

		GLX::AnimationScope scope(GetConfig<bool>("view.allow_init_animations", true));


		Unretained <GLX::Object> view = kNoValue;

		if (System::kEnvironmentType == System::kEnvironmentTypeDesktopApp && GLX::kIsMobile)
		{
			Detail::g_create_stylesheet_options = [](const GLX::Object & client)
			{
				auto isize = EmulatedMobileWrapper::GetEmulatedDevice().size;

				if (EmulatedMobileWrapper::IsLandscape()) Swap(isize.w, isize.h);

				return CreateStylesheetOptions(GLX::kSystemTheme.a, GLX::kSystemTheme.b, isize, false);
			};

			view = New<EmulatedMobileWrapper>(ctr(app));
		}
		else
		{
			view = ctr(app);
		}

		if (Data::GetBool(view, GLX::kresizable))
		{
			window_flags |= System::kWindowStyleResizable;

			if (System::kEnvironmentType == System::kEnvironmentTypeAudioPlugin)
			{
				window_flags |= System::kWindowStyleExPluginResizeHandle;
			}
		}

		Unretained <GLX::WindowClient> client = kNoValue;

		switch (System::kEnvironmentType)
		{
		case System::kEnvironmentTypeAudioPlugin:
		case System::kEnvironmentTypeLibrary:
			client = New<PluginWindowClient>(*session, view);
			break;

		case System::kEnvironmentTypeDesktopApp:
			client = New<DesktopAppWindowClient>(view);
			break;

		default:
			client = New<MobileWindowClient>(view);
			break;
		}

		GLX::FocusBranch(view);

		return client;
	};
}

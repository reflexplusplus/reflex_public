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
	WindowClient(System::Window & owner)
		: GLX::WindowClient(global->IdeEnabled())
	{
		owner.SetClient(this);
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
	public Streamable
{
	PluginWindowClient(System::Window & owner, File::PersistentPropertySet & session, GLX::Object & view)
		: WindowClient(owner)
		, Streamable(session, "bootstrap.plugin_window_size", 1)
		, m_resizable(Data::GetBool(view, GLX::kresizable))
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

	DesktopAppWindowClient(System::Window & owner, GLX::Object & view)
		: WindowClient(owner)
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

	MobileWindowClient(System::Window & owner, GLX::Object & view)
		: WindowClient(owner)
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
				IDE::Detail::RestoreStreamable(propertyset, {}, i.b);

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
				IDE::Detail::StoreStreamable(propertyset, selector->GetPanel(idx));
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

	void OnSetRect(System::WindowDisplay state, const System::iRect & rect, const System::iRect & interactable, Int32 dpifactor) override
	{
		auto style = Make<GLX::Style>(kNullKey);

		auto padding = GLX::MakeMargin(ToFloat32(interactable.origin.x), ToFloat32(interactable.origin.y), ToFloat32(rect.size.w - interactable.size.w), ToFloat32(rect.size.h - interactable.size.h));

		GLX::SetMargin(style, "padding", padding);

		GetContent()->SetMod("client_area", GLX::Detail::ComputedStyle::Create(style));

		GLX::WindowClient::OnSetRect(state, rect, interactable, dpifactor);
	}

	TRef <GLX::Object> m_view;
};

struct EmulatedMobileWrapper : public GLX::Object
{
	using Device = Tuple <CString::View, System::iSize>;

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
		, m_magnify_out(L"-")
		, m_magnify_in(L"+")
		, m_landscape(L"LS")
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

		SelectDevice(GetEmulatedDevice(), IsLandscape());
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

	bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (auto menu = GLX::GetMenu(e); menu && src == m_popup)
		{
			auto current_idx = GetEmulatedDeviceIndex();

			UInt idx = 0;

			for (auto [name, size] : kDevices)
			{
				GLX::AddMenuOption(menu, ToWString(name), idx++ == current_idx);
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
		auto [name,isize] = device;

		GLX::SetText(m_popup, ToWString(name));
		GLX::SetText(m_popup, Join(ToWString(isize.w), L'x', ToWString(isize.h)), "info");
		GLX::Select(m_landscape, landscape);

		auto fsize = MakeSize(ToFloat32(isize.w), ToFloat32(isize.h));

		if (landscape) Swap(fsize.w, fsize.h);

		GLX::SetBounds(m_view, "mobile_emulation", fsize, fsize);
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

		Array <Address> adrs;

		lock.Enumerate(GetTypeID<GLX::StyleSheet>(), [&lock, &adrs](const File::ResourcePool::Token & token)
		{
			adrs.Push(token.address);
		});

		for (auto adr : adrs)
		{
			lock.Remove(adr);
		}

		GetWindow()->AutoFit();
	}



	GLX::Button m_landscape, m_magnify_out, m_magnify_in;

	GLX::Popup m_popup;

	GLX::Object m_header;

	TRef <GLX::Object> m_view;


	static constexpr Key32 kMobileDevice = "bootstrap.mobile_device";
	static constexpr Key32 kMagnification = "bootstrap.mobile_magnification";
	static constexpr Key32 kLandscape = "bootstrap.mobile_landscape";

	static constexpr Tuple <CString::View, System::iSize> kDevices[] =
	{
		{ "Samsung Galaxy S7", { 360, 640 } },
		{ "iPhone SE", { 375, 647 } },
		{ "iPhone 13 mini", { 375, 700 } },
		{ "Pixel 8", { 360, 712 } },
		{ "iPhone 13", { 390, 730 } },
		{ "iPhone 15 / Pro", { 393, 734 } },
		{ "iPhone 17", { 402, 756 } },
		{ "iPhone 15 Plus / Pro Max", { 430, 814 } },
		{ "iPhone 17 Pro Max", { 440, 838 } },
		{ "Samsung Galaxy A25", { 360, 780 - 50 } },
		{ "Samsung Galaxy S8+", { 360, 740 } },
		{ "Samsung Galaxy A51", { 360, 800 - 24 } },
		{ "iPad Mini", { 744, 1085 } },
		{ "iPad", { 820, 1132 } },
	};
};

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::GLX::WindowClient> Reflex::Bootstrap::Detail::CreateAppWindow(System::Window & window, GLX::Object & view)
{
	auto window_client = New<WindowClient>(window);

	window_client->SetContent(view);

	return window_client;
}

void Reflex::Bootstrap::Detail::PublishAppView(System::App::Configuration & config, const Function <TRef<GLX::Object>(Object & instance_delegate)> & ctr)
{
	config.view_ctr = [ctr](System::App & system, void * host_window)
	{
		auto config = AcquireProperty<Data::MapOfKey32Property<UInt32>>(global->prefs, kViewGraphicsConfig);	//restore graphics settings

		auto glx = GLX::Start(global->resourcepool, config->value);	//start GLX, the window will retain the object, so we dont need to store

		auto app = system.GetClient();

		auto session = app->QueryProperty<File::PersistentPropertySet>("session", Bootstrap::global->prefs.Adr());	//need this backdoor temporarily, to support legacy non-App clients

		TRef <GLX::WindowClient> client = kNoValue;

		TRef <GLX::Object> view = kNoValue;

		UInt8 window_flags = System::kWindowStyleFrame | System::kWindowStyleMinimisable;

		GLX::Core::Context ctx;

		GLX::AnimationScope scope(GetConfig<bool>("view.allow_init_animations", true));

		if (System::kEnvironmentType == System::kEnvironmentTypeDesktopApp && GLX::kIsMobile)
		{
			Detail::g_create_stylesheet_options = []()
			{
				auto [name,isize] = EmulatedMobileWrapper::GetEmulatedDevice();

				if (EmulatedMobileWrapper::IsLandscape()) Swap(isize.w, isize.h);

				return CreateStylesheetOptions(GLX::kSystemTheme.a, GLX::kSystemTheme.b, isize);
			};

			view = New<EmulatedMobileWrapper>(ctr(app));
		}
		else
		{
			view = ctr(app);

			window_flags |= Data::GetBool(view, GLX::kresizable) ? System::kWindowStyleResizable : 0;
		}

		auto window = System::Window::Create(window_flags, false, host_window);

		switch (System::kEnvironmentType)
		{
		case System::kEnvironmentTypeAudioPlugin:
		case System::kEnvironmentTypeLibrary:
			client = New<PluginWindowClient>(window, *session, view);
			break;

		case System::kEnvironmentTypeDesktopApp:
			client = New<DesktopAppWindowClient>(window, view);
			break;

		default:
			client = New<MobileWindowClient>(window, view);
			break;
		}

		client->SetTitle(ToWString(global->product));

		GLX::FocusBranch(view);

		return window;
	};
}

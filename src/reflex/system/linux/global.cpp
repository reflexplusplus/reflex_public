#include "library.h"

REFLEX_BEGIN_INTERNAL(Reflex::System::Linux)

WString MakePath(const WString::View & path)
{
	if (const char * phome = getenv("HOME"))
	{
		auto home = ToWString(CString::View(phome));

		if (home.GetLast() != kPathDelimiter) home.Push(kPathDelimiter);

		return Join(home, path);
	}

	return {};
}

REFLEX_END_INTERNAL

Reflex::System::Linux::Library::Library()
{
	curl_global_init(CURL_GLOBAL_DEFAULT);

#if REFLEX_INCLUDE_UI
	InitialiseUI();
#endif
}

Reflex::System::Linux::Library::~Library()
{
#if REFLEX_INCLUDE_UI
	DeinitialiseUI();
#endif

	curl_global_cleanup();
}

#if REFLEX_INCLUDE_UI
bool Reflex::System::Linux::Library::InitialiseUI()
{
	m_display = wl_display_connect(nullptr);

	if (!m_display)
	{
		DEV_WARN("Wayland display connection failed");
		return false;
	}

	m_registry = wl_display_get_registry(m_display);

	if (!m_registry)
	{
		DEV_WARN("Wayland registry acquisition failed");
		return false;
	}

	static const wl_registry_listener k_registry_listener =
	{
		&Library::RegistryGlobal,
		&Library::RegistryGlobalRemove
	};

	wl_registry_add_listener(m_registry, &k_registry_listener, this);

	wl_display_roundtrip(m_display);

	return HasUI();
}

void Reflex::System::Linux::Library::DeinitialiseUI()
{
#if REFLEX_LINUX_HAS_XDG_SHELL
	if (m_xdg_wm_base)
	{
		xdg_wm_base_destroy(m_xdg_wm_base);
		m_xdg_wm_base = nullptr;
	}
#endif

	if (m_shell)
	{
		wl_shell_destroy(m_shell);
		m_shell = nullptr;
	}

	if (m_compositor)
	{
		wl_compositor_destroy(m_compositor);
		m_compositor = nullptr;
	}

	if (m_registry)
	{
		wl_registry_destroy(m_registry);
		m_registry = nullptr;
	}

	if (m_display)
	{
		wl_display_disconnect(m_display);
		m_display = nullptr;
	}
}

void Reflex::System::Linux::Library::RegistryGlobal(void * data, wl_registry * registry, UInt32 name, const char * interface_name, UInt32 version)
{
	auto self = Reinterpret<Library>(data);
	CString::View interface = interface_name;

	if (interface == "wl_compositor")
	{
		self->m_compositor = Reinterpret<wl_compositor>(wl_registry_bind(registry, name, &wl_compositor_interface, Min<UInt32>(version, 4)));
	}
	else if (interface == "wl_shell")
	{
		self->m_shell = Reinterpret<wl_shell>(wl_registry_bind(registry, name, &wl_shell_interface, 1));
	}
#if REFLEX_LINUX_HAS_XDG_SHELL
	else if (interface == "xdg_wm_base")
	{
		self->m_xdg_wm_base = Reinterpret<xdg_wm_base>(wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));

		static const xdg_wm_base_listener k_xdg_listener =
		{
			&Library::XdgPing
		};

		xdg_wm_base_add_listener(self->m_xdg_wm_base, &k_xdg_listener, self);
		self->m_has_xdg_shell = true;
	}
#endif
}

void Reflex::System::Linux::Library::RegistryGlobalRemove(void * data, wl_registry * registry, UInt32 name)
{
}

#if REFLEX_LINUX_HAS_XDG_SHELL
void Reflex::System::Linux::Library::XdgPing(void * data, xdg_wm_base * shell, UInt32 serial)
{
	xdg_wm_base_pong(shell, serial);
}
#endif
#endif

const Reflex::System::Platform Reflex::System::kPlatform = Reflex::System::kPlatformLinux;

Reflex::TRef <Reflex::Object> Reflex::System::CreateListener(Notification id, void * client, void(*callback)(void*))
{
	return Object::null;
}

Reflex::CString Reflex::System::GetOperatingSystemVersion()
{
	struct utsname info;

	if (uname(&info) == 0)
	{
		return Join(info.sysname, " ", info.release);
	}

	return "Linux";
}

Reflex::UInt64 Reflex::System::GetSystemID()
{
	return UInt64(gethostid());
}

Reflex::UInt32 Reflex::System::GetNumProcessor()
{
	long n = sysconf(_SC_NPROCESSORS_ONLN);

	if (n > 0)
	{
		return UInt32(n);
	}

	return 1;
}

Reflex::WString Reflex::System::GetPath(Path path)
{
	switch (path)
	{
	case kPathTemp:
		return L"/tmp/";

	case kPathDesktop:
		return Linux::MakePath(L"Desktop/");

	case kPathApplicationData:
		return Linux::MakePath(L".config/");

	case kPathUserData:
		return Linux::MakePath(L".local/share/");

	case kPathUserDocuments:
		return Linux::MakePath(L"Documents/");
	}

	return {};
}

Reflex::WString Reflex::System::GetExecutablePath()
{
	WString rtn;

	char buffer[PATH_MAX] = {};

	auto length = readlink("/proc/self/exe", buffer, PATH_MAX - 1);

	if (length <= 0)
	{
		return rtn;
	}

	buffer[length] = 0;

	Data::DecodeUTF8(rtn, { Reinterpret<UInt8>(buffer), UInt(length) });

	return rtn;
}

Reflex::CString Reflex::System::GetLanguage()
{
	if (const char * language = getenv("LANG"))
	{
		CString::View view = language;

		if (auto dot = Search(view, '.')) view = Left(view, dot.value);
		if (auto underscore = Search(view, '_')) view = Left(view, underscore.value);

		return view;
	}

	return {};
}

bool Reflex::System::Open(const WString & path)
{
	auto utf8 = Common::ToUTF8(path);

	return system(utf8.GetData()) == 0;
}

bool Reflex::System::Share(const ArrayView<WString>&, const WString&)
{
	return false;
}

Reflex::Float64 Reflex::System::GetElapsedTime()
{
	timespec ts = {};

	if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
	{
		return Float64(ts.tv_sec) + (Float64(ts.tv_nsec) / 1'000'000'000.0);
	}

	return 0.0;
}

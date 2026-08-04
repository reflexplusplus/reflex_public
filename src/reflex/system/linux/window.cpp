#include "window.h"

REFLEX_NS(Reflex::System::Linux)

static const wl_shell_surface_listener k_shell_surface_listener =
{
	&Window::ShellPing,
	&Window::ShellConfigure,
	&Window::ShellPopupDone
};

#if REFLEX_LINUX_HAS_XDG_SHELL
static const xdg_surface_listener k_xdg_surface_listener =
{
	&Window::XdgSurfaceConfigure
};

static const xdg_toplevel_listener k_xdg_toplevel_listener =
{
	&Window::XdgToplevelConfigure,
	&Window::XdgToplevelClose
};
#endif

Window::Window(UInt32 flags, bool topmost, void * host_window)
	: m_styleflags(flags)
	, m_topmost(topmost)
	, m_host_window(host_window)
{
	if (!globals->HasUI() || host_window)
	{
		if (host_window)
		{
			DEV_WARN("Wayland host-window embedding is not implemented yet");
		}

		return;
	}

	m_surface = wl_compositor_create_surface(globals->m_compositor);

	if (!m_surface)
	{
		DEV_WARN("Wayland surface creation failed");
		return;
	}

#if REFLEX_LINUX_HAS_XDG_SHELL
	if (globals->m_xdg_wm_base)
	{
		m_xdg_surface = xdg_wm_base_get_xdg_surface(globals->m_xdg_wm_base, m_surface);
		m_xdg_toplevel = xdg_surface_get_toplevel(m_xdg_surface);

		xdg_surface_add_listener(m_xdg_surface, &k_xdg_surface_listener, this);
		xdg_toplevel_add_listener(m_xdg_toplevel, &k_xdg_toplevel_listener, this);

		ApplyTitle();
	}
	else
#endif
	if (globals->m_shell)
	{
		m_shell_surface = wl_shell_get_shell_surface(globals->m_shell, m_surface);

		if (m_shell_surface)
		{
			wl_shell_surface_add_listener(m_shell_surface, &k_shell_surface_listener, this);
			wl_shell_surface_set_toplevel(m_shell_surface);
		}
	}

	Commit();
}

Window::~Window()
{
	if (m_egl_window)
	{
		wl_egl_window_destroy(m_egl_window);
		m_egl_window = nullptr;
	}

#if REFLEX_LINUX_HAS_XDG_SHELL
	if (m_xdg_toplevel)
	{
		xdg_toplevel_destroy(m_xdg_toplevel);
		m_xdg_toplevel = nullptr;
	}

	if (m_xdg_surface)
	{
		xdg_surface_destroy(m_xdg_surface);
		m_xdg_surface = nullptr;
	}
#endif

	if (m_shell_surface)
	{
		wl_shell_surface_destroy(m_shell_surface);
		m_shell_surface = nullptr;
	}

	if (m_surface)
	{
		wl_surface_destroy(m_surface);
		m_surface = nullptr;
	}
}

void Window::SetClient(TRef <Client> client)
{
	m_client = client;

	if (client)
	{
		client->OnSetOwner(this);
		NotifyRect();
	}
}

void Window::SetTitle(const WString & label)
{
	m_title = label;
	ApplyTitle();
}

void Window::SetDisplayMode(WindowDisplay mode)
{
	m_display_mode = mode;

#if REFLEX_LINUX_HAS_XDG_SHELL
	if (m_xdg_toplevel)
	{
		switch (mode)
		{
		case kWindowDisplayFullScreen:
			xdg_toplevel_set_fullscreen(m_xdg_toplevel, nullptr);
			break;

		case kWindowDisplayMinimised:
			xdg_toplevel_set_minimized(m_xdg_toplevel);
			break;

		default:
			xdg_toplevel_unset_fullscreen(m_xdg_toplevel);
			break;
		}
	}
#endif

	if (m_shell_surface)
	{
		if (mode == kWindowDisplayFullScreen)
		{
			wl_shell_surface_set_fullscreen(m_shell_surface, WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT, 0, nullptr);
		}
		else
		{
			wl_shell_surface_set_toplevel(m_shell_surface);
		}
	}

	Commit();
	NotifyRect();
}

void Window::SetRect(const iRect & rect)
{
	m_rect = rect;
	m_rect.size.w = Max(m_rect.size.w, 1);
	m_rect.size.h = Max(m_rect.size.h, 1);

	ResizeEGLWindow(m_rect.size);
	Commit();
	NotifyRect();
}

wl_egl_window * Window::AcquireEGLWindow()
{
	if (!m_surface)
	{
		return nullptr;
	}

	if (!m_egl_window)
	{
		m_egl_window = wl_egl_window_create(m_surface, m_rect.size.w, m_rect.size.h);
	}

	return m_egl_window;
}

void Window::ResizeEGLWindow(const iSize & size)
{
	if (m_egl_window)
	{
		wl_egl_window_resize(m_egl_window, Max(size.w, 1), Max(size.h, 1), 0, 0);
	}
}

void Window::ApplyTitle()
{
	auto utf8 = Common::ToUTF8(m_title);

#if REFLEX_LINUX_HAS_XDG_SHELL
	if (m_xdg_toplevel)
	{
		xdg_toplevel_set_title(m_xdg_toplevel, utf8.GetData());
	}
#endif

	if (m_shell_surface)
	{
		wl_shell_surface_set_title(m_shell_surface, utf8.GetData());
	}
}

void Window::NotifyRect()
{
	if (m_client)
	{
		m_client->OnSetRect(m_display_mode, m_rect, m_rect, 1);
	}
}

void Window::Commit()
{
	if (m_surface)
	{
		wl_surface_commit(m_surface);

		if (globals->m_display)
		{
			wl_display_flush(globals->m_display);
		}
	}
}

void Window::ShellPing(void * data, wl_shell_surface * shell_surface, UInt32 serial)
{
	wl_shell_surface_pong(shell_surface, serial);
}

void Window::ShellConfigure(void * data, wl_shell_surface * shell_surface, UInt32 edges, Int32 width, Int32 height)
{
	auto self = Reinterpret<Window>(data);

	if (width > 0 && height > 0)
	{
		self->m_rect.size = { width, height };
		self->ResizeEGLWindow(self->m_rect.size);
		self->NotifyRect();
	}
}

void Window::ShellPopupDone(void * data, wl_shell_surface * shell_surface)
{
}

#if REFLEX_LINUX_HAS_XDG_SHELL
void Window::XdgSurfaceConfigure(void * data, xdg_surface * surface, UInt32 serial)
{
	auto self = Reinterpret<Window>(data);

	xdg_surface_ack_configure(surface, serial);

	self->Commit();
}

void Window::XdgToplevelConfigure(void * data, xdg_toplevel * toplevel, Int32 width, Int32 height, wl_array * states)
{
	auto self = Reinterpret<Window>(data);

	if (width > 0 && height > 0)
	{
		self->m_rect.size = { width, height };
		self->ResizeEGLWindow(self->m_rect.size);
		self->NotifyRect();
	}
}

void Window::XdgToplevelClose(void * data, xdg_toplevel * toplevel)
{
	auto self = Reinterpret<Window>(data);

	if (self->m_client)
	{
		self->m_client->OnRequestClose();
	}
}
#endif

REFLEX_END

Reflex::Int32 Reflex::System::GetMaxPixelDensity()
{
	return 1;
}

Reflex::Array <Reflex::System::iRect> Reflex::System::GetScreens()
{
	return { { { 0, 0 }, { 1280, 720 } } };
}

bool Reflex::System::IsDarkTheme()
{
	return false;
}

Reflex::Float Reflex::System::GetFontScale()
{
	return 1.0f;
}

Reflex::UInt8 Reflex::System::GetModifierKeys()
{
	return 0;
}

void Reflex::System::SetClipboard(const WString & string)
{
}

Reflex::WString Reflex::System::GetClipboard()
{
	return {};
}

Reflex::TRef <Reflex::System::Window> Reflex::System::Window::Create(UInt32 styleflags, bool topmost, void * systemparent)
{
	return REFLEX_CREATE(Linux::Window, styleflags, topmost, systemparent);
}

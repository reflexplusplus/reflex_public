#pragma once

#include "wayland.h"

REFLEX_NS(Reflex::System::Linux)

struct Library
{
	Library();

	~Library();

#if REFLEX_INCLUDE_UI
	bool InitialiseUI();

	void DeinitialiseUI();

	bool HasUI() const { return m_display && m_compositor && (m_shell || m_has_xdg_shell); }

	static void RegistryGlobal(void * data, wl_registry * registry, UInt32 name, const char * interface_name, UInt32 version);

	static void RegistryGlobalRemove(void * data, wl_registry * registry, UInt32 name);

#if REFLEX_LINUX_HAS_XDG_SHELL
	static void XdgPing(void * data, xdg_wm_base * shell, UInt32 serial);
#endif

	wl_display * m_display = nullptr;
	wl_registry * m_registry = nullptr;
	wl_compositor * m_compositor = nullptr;
	wl_shell * m_shell = nullptr;

#if REFLEX_LINUX_HAS_XDG_SHELL
	xdg_wm_base * m_xdg_wm_base = nullptr;
#endif

	bool m_has_xdg_shell = false;
	volatile bool m_quit = false;
#endif
};

inline Reflex::Detail::Module::Member <Library> globals(Common::g_module);

REFLEX_END

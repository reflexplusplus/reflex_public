#pragma once

#include "library.h"

#if REFLEX_INCLUDE_UI
#include <wayland-egl.h>
#endif

REFLEX_NS(Reflex::System::Linux)

class Window : public System::Window
{
public:

	REFLEX_OBJECT(System::Linux::Window, System::Window);

	Window(UInt32 flags, bool topmost, void * host_window);

	~Window() override;


	void SetClient(TRef <Client> client) override;

	TRef <Client> GetClient() override { return m_client; }

	void Attach(System::Window & parent) override {}

	void Detach() override {}

	void SetTitle(const WString & label) override;

	void SetDisplayMode(WindowDisplay mode) override;

	void SetRect(const iRect & rect) override;

	void SendTop() override {}

	void EnableInput(bool enable) override {}

	void SetMouseCursor(MouseCursor mouse_cursor) override {}

	void SetMousePosition(const iPoint & point) override {}

	void BeginDragDropFiles(const ArrayView <WString> & filenames) override {}

	TRef <ObjectOf <RawBitmap> > CreateExportBitmapBuffer(UInt8 flags) const override { return kNoValue; }

	void ExportBitmap(TRef <ObjectOf <RawBitmap> > buffer) const override {}

	void * GetSystemHandle() override { return this; }


	wl_surface * GetSurface() const { return m_surface; }

	wl_egl_window * AcquireEGLWindow();

	void ResizeEGLWindow(const iSize & size);

	const iRect & GetRect() const { return m_rect; }


	static void ShellPing(void * data, wl_shell_surface * shell_surface, UInt32 serial);

	static void ShellConfigure(void * data, wl_shell_surface * shell_surface, UInt32 edges, Int32 width, Int32 height);

	static void ShellPopupDone(void * data, wl_shell_surface * shell_surface);

#if REFLEX_LINUX_HAS_XDG_SHELL
	static void XdgSurfaceConfigure(void * data, xdg_surface * surface, UInt32 serial);

	static void XdgToplevelConfigure(void * data, xdg_toplevel * toplevel, Int32 width, Int32 height, wl_array * states);

	static void XdgToplevelClose(void * data, xdg_toplevel * toplevel);
#endif


private:

	void ApplyTitle();

	void NotifyRect();

	void Commit();


	Reference <Client> m_client;

	UInt32 m_styleflags = 0;

	bool m_topmost = false;

	WindowDisplay m_display_mode = kWindowDisplayMinimised;

	iRect m_rect = { { 64, 64 }, { 640, 480 } };

	WString m_title;

	void * m_host_window = nullptr;

	wl_surface * m_surface = nullptr;

	wl_shell_surface * m_shell_surface = nullptr;

#if REFLEX_LINUX_HAS_XDG_SHELL
	xdg_surface * m_xdg_surface = nullptr;
	xdg_toplevel * m_xdg_toplevel = nullptr;
#endif

	wl_egl_window * m_egl_window = nullptr;
};

REFLEX_END

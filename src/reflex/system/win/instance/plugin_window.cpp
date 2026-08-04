#include "../library.h"
#include "plugin_window.h"





//
//pluginwindpw

REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

bool IsFocused(HWND desktop, HWND focus, HWND child, UInt maxdepth = 4)
{
	if (focus == child) return true;

	auto itr = child;

	while (itr = GetParent(itr))
	{
		if (itr == desktop) return false;

		if (itr == focus) return true;

		if (!maxdepth--) return false;
	}

	return false;
}

class Vst2PluginWindow : public PluginWindow
{
public:

	Vst2PluginWindow(Common::PluginInstance & plugin, HWND hostwindow)
		: PluginWindow(plugin, false, hostwindow)
		, m_resizehostwindow([](Vst2PluginWindow &, Int32, Int32) {})
	{
		constexpr CString::View kSteinberg = "Steinberg";

		if (Left<true>(Common::g_plugin_host, kSteinberg.size) == kSteinberg)	//Cubase simply does not respond to VST resize call
		{
			m_resizehostwindow = &ResizeShittyCubaseWindow;

			auto density = globals->m_maxpixeldensity;

			RECT rect;

			GetClientRect(hostwindow, &rect);

			if (rect.right > 256)
			{
				m_xywh.size = { rect.right - rect.left, rect.bottom - rect.top };

				if (density > 1)
				{
					m_xywh.size.w += density;

					m_xywh.size.h += density;
				}

				m_xywh.size.w /= density;

				m_xywh.size.h /= density;
			}
		}
	}




private:

	virtual void RequestResize(const iSize & size, Int32 wdelta, Int32 hdelta) override
	{
		plugin.OnSetViewSize(size);

		m_resizehostwindow(*this, wdelta, hdelta);
	}

	static void ResizeShittyCubaseWindow(Vst2PluginWindow & self, Int32 w_delta, Int h_delta);


	FunctionPointer <void(Vst2PluginWindow &, Int32, Int32)> m_resizehostwindow;

};

class Vst3AndClapPluginWindow : public PluginWindow
{
public:

	using PluginWindow::PluginWindow;



private:

	virtual void RequestResize(const iSize & size, Int32 wdelta, Int32 hdelta) override
	{
		plugin.OnSetViewSize(size);
	}

};

void Vst2PluginWindow::ResizeShittyCubaseWindow(Vst2PluginWindow & self, Int32 w_delta, Int h_delta)
{
	Int32 density = globals->m_maxpixeldensity;

	if (auto parent = GetAncestor(self.m_hwnd, GA_PARENT))
	{
		RECT rect;

		::GetWindowRect(parent, &rect);

		auto w = QuantiseUp<Int>(rect.right - rect.left, density);

		auto h = QuantiseUp<Int>(rect.bottom - rect.top, density);

		SetWindowPos(parent, 0, 0, 0, w + w_delta, h + h_delta, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
	}
}

REFLEX_END_INTERNAL

Reflex::System::Win::PluginWindow::PluginWindow(Common::PluginInstance & plugin, bool resizable, HWND hostwindow)
	: Common::PluginWindow<Window>(plugin, resizable, hostwindow, 0, false, true)
{
	if (globals->m_plugin_windows.Empty())
	{
		UInt32 threadid = GetCurrentThreadId();

		globals->m_hooks[0] = SetWindowsHookEx(WH_MOUSE, &PluginWindow::MouseHookProc, Library::st_hinstance, threadid);

		globals->m_hooks[1] = SetWindowsHookEx(WH_KEYBOARD, &PluginWindow::KeyboardHookProc, Library::st_hinstance, threadid);
	}

	globals->m_plugin_windows.Push(this);

	m_xywh.size = { 0, 0 };

	constexpr CString::View kPreSonus = "PreSonus";

	if (Left<true>(Common::g_plugin_host, kPreSonus.size) == kPreSonus)
	{
		st_max_window_depth = 1;
	}
}

Reflex::System::Win::PluginWindow::~PluginWindow()
{
	Remove(globals->m_plugin_windows, this);

	if (globals->m_plugin_windows.Empty())
	{
		UnhookWindowsHookEx(globals->m_hooks[0]);

		UnhookWindowsHookEx(globals->m_hooks[1]);
	}
}

void Reflex::System::Win::PluginWindow::SetRect(const iRect & rect)
{
	auto density = globals->m_maxpixeldensity;

	Int32 w_delta = (rect.size.w - m_xywh.size.w) * density;

	Int32 h_delta = (rect.size.h - m_xywh.size.h) * density;

	m_xywh.size = rect.size;

	auto w = rect.size.w * density;

	auto h = rect.size.h * density;

	SetWindowPos(m_hwnd, 0, 0, 0, w, h, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);

	if (w_delta | h_delta)
	{
		RequestResize({ w, h }, w_delta /** density*/, h_delta /** density*/);
	}
}

void Reflex::System::Win::PluginWindow::SetDisplayMode(WindowDisplay mode)
{
	auto density = globals->m_maxpixeldensity;

	m_state = kWindowDisplayWindowed;

	POINT point = { 0, 0 };

	if (auto parent = GetAncestor(m_hwnd, GA_PARENT))
	{
		MapWindowPoints(m_hwnd, parent, &point, 1);
	}

	MoveWindow(m_hwnd, point.x, point.y, m_xywh.size.w * density, m_xywh.size.h * density, 1);

	ShowWindow(m_hwnd, SW_SHOWNORMAL);
}

LRESULT CALLBACK Reflex::System::Win::PluginWindow::KeyboardHookProc(int code, WPARAM wparam, LPARAM lparam)
{
	auto vk = UInt(wparam);

	if (vk < 256)
	{
		bool keyup = BitCheck(lparam, 31);

		HWND desktop = GetDesktopWindow();

		HWND focus = GetFocus();

		REFLEX_RFOREACH(window, globals->m_plugin_windows)
		{
			if (IsFocused(desktop, focus, window->m_hwnd, st_max_window_depth))
			{
				if (keyup)
				{
					window->ProcessKeyUp(vk);

					return -1;
				}
				else if (window->ProcessKeyDown(vk))
				{
					auto kbs = globals->m_kbs;

					GetKeyboardState(kbs);

					UINT uparam = UINT(vk);

					WORD character = 0;

					ToAscii(uparam, MapVirtualKeyW(uparam, MAPVK_VK_TO_VSC), kbs, &character, 0);

					window->m_client->OnCharacter(character);

					return -1;
				}

				break;
			}
		}
	}

	return CallNextHookEx(0, code, wparam, lparam);
}

LRESULT CALLBACK Reflex::System::Win::PluginWindow::MouseHookProc(int code, WPARAM wparam, LPARAM lparam)
{
	if (wparam == WM_MOUSEWHEEL && True(globals->m_mouseover))
	{
		Window & window = *globals->m_mouseover;

		if (IsFocused(GetDesktopWindow(), GetFocus(), window.m_hwnd))
		{
			MOUSEHOOKSTRUCTEX * data = Reinterpret<MOUSEHOOKSTRUCTEX*>(lparam);

			(*window.m_msgfn[kMOUSEWHEEL_Y])(window, data->mouseData, 0);
		}
	}

	return CallNextHookEx(0, code, wparam, lparam);
}

Reflex::TRef <Reflex::System::Window> Reflex::System::Window::Create(UInt32 styleflags, bool ontop, void * hostwindow)
{
	if (auto plugin = Common::PluginInstance::IsOpeningPluginWindow(hostwindow))
	{
		switch (plugin->session->format)
		{
		case Common::kPluginFormatVST3:
		case Common::kPluginFormatCLAP:
			return REFLEX_CREATE(Win::Vst3AndClapPluginWindow, *plugin, (styleflags & kWindowStyleResizable), reinterpret_cast<HWND>(hostwindow));

		default:
			return REFLEX_CREATE(Win::Vst2PluginWindow, *plugin, reinterpret_cast<HWND>(hostwindow));
		}
	}
	else
	{
		return REFLEX_CREATE(Win::Window, reinterpret_cast<HWND>(hostwindow), styleflags, ontop, false);
	}
}

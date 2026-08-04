#include "dpi.h"
#include <ShellScalingApi.h>




REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

typedef BOOL(WINAPI *PFN_SetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);

typedef HRESULT(WINAPI *PFN_GetDpiForMonitor)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);

HRESULT __stdcall LegacyGetMonitorDpi(HMONITOR, MONITOR_DPI_TYPE, UINT * dpix, UINT * dpiy)
{
	HWND hwnd = GetDesktopWindow();

	RECT rect;

	GetClientRect(hwnd, &rect);

	if (rect.right > 1920 && rect.bottom > 1080)
	{
		HDC screen = GetDC(hwnd);

		auto dpi = GetDeviceCaps(screen, LOGPIXELSX);

		*dpix = dpi;
		*dpiy = dpi;

		ReleaseDC(hwnd, screen);
	}

	return 0;
}

iRect ToRect(const RECT & rc, Int density)
{
	iRect r;

	r.origin.x = Int32(rc.left) / density;
	r.origin.y = Int32(rc.top) / density;

	r.size.w = (rc.right - rc.left) / density;
	r.size.h = (rc.bottom - rc.top) / density;

	return r;
}

UInt ChooseTierFromRaw(Float raw_dpi)
{
	if (raw_dpi < 140.0f) return 1;           // 96-139 dpi
	if (raw_dpi < 220.0f) return 2;           // 140-219 dpi
	if (raw_dpi < 300.0f) return 3;           // 220-299 dpi

	return 4;                                  //>=300 dpi
}

REFLEX_END_INTERNAL

Reflex::System::Win::MonitorInfo::MonitorInfo(HINSTANCE hinstance)
{
	HMODULE user32 = LoadLibraryW(L"user32.dll");

	if (hinstance)
	{
		if (auto SetProcessDpiAwarenessContext = reinterpret_cast<PFN_SetProcessDpiAwarenessContext>(GetProcAddress(user32, "SetProcessDpiAwarenessContext")))
		{
			SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		}
		else
		{
			SetProcessDPIAware();
		}
	}

	if (auto GetDpiForWindow = GetProcAddress(user32, "GetDpiForWindow"))
	{
		m_get_monitor_dpi_fn = GetDpiForWindow;
	}
	else
	{
		m_get_monitor_dpi_fn = &LegacyGetMonitorDpi;
	}

	Refresh();
}

Reflex::Pair <Reflex::Int, Reflex::Float> Reflex::System::Win::MonitorInfo::GetMaxPixelDensity() const
{
	if (m_monitors.Empty())
	{
		return { 1, 1.0f };
	}

	const Monitor & best = *std::max_element(m_monitors.begin(), m_monitors.end(), [](const Monitor & a, const Monitor & b) 
	{ 
		return a.effective_scale < b.effective_scale; 
	});

	return { Int(best.tier), best.adjustment };
}

Reflex::Array < Reflex::Pair < Reflex::System::iRect, Reflex::Float > > Reflex::System::Win::MonitorInfo::GetScreens() const
{
	Array< Pair<iRect, Float> > out;

	REFLEX_FOREACH(m, m_monitors)
	{
		out.Push({ ToRect(m.rc, m_master_pixel_density), m.adjustment });
	}

	return out;
}

void Reflex::System::Win::MonitorInfo::Refresh()
{
	m_monitors.Clear();
		
	EnumDisplayMonitors(nullptr, nullptr, &MonitorInfo::EnumProc, reinterpret_cast<LPARAM>(this));

	m_master_pixel_density = 1;
		
	REFLEX_FOREACH(m, m_monitors)
	{
		m_master_pixel_density = Max(m_master_pixel_density, m.tier);
	}
}

BOOL Reflex::System::Win::MonitorInfo::EnumProc(HMONITOR hmon, HDC, LPRECT, LPARAM ctx)
{
	reinterpret_cast<MonitorInfo*>(ctx)->AddMonitorX(hmon);

	return TRUE;
}

void Reflex::System::Win::MonitorInfo::AddMonitorX(HMONITOR hmon)
{
	MONITORINFOEXW mi = {};

	mi.cbSize = sizeof(mi);
	
	if (!GetMonitorInfoW(hmon, &mi)) return;

	UINT eff_x = 96, eff_y = 96;
		
	UINT raw_x = 0, raw_y = 0;

	Float effective_dpi = 96.0f;
		
	Float raw_dpi = 0.0f;

	auto GetDpiForMonitor = reinterpret_cast<PFN_GetDpiForMonitor>(m_get_monitor_dpi_fn);

	if (SUCCEEDED(GetDpiForMonitor(hmon, MDT_EFFECTIVE_DPI, &eff_x, &eff_y)))
	{
		effective_dpi = Float(eff_x);
	}

	if (SUCCEEDED(GetDpiForMonitor(hmon, MDT_RAW_DPI, &raw_x, &raw_y)))
	{
		raw_dpi = Float(raw_x);
	}

	const Float effective_scale = effective_dpi / 96.0f;
	const UInt  tier = ChooseTierFromRaw(raw_dpi > 0.0f ? raw_dpi : effective_dpi);
	const Float adjustment = effective_scale / Float(tier);

	Monitor m;
	m.rc = mi.rcMonitor;
	m.raw_dpi = raw_dpi;
	m.effective_dpi = effective_dpi;
	m.effective_scale = effective_scale;
	m.tier = tier;
	m.adjustment = adjustment;

	m_monitors.Push(m);
}


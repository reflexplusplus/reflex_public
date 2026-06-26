#pragma once

#include "../[require].h"




REFLEX_NS(Reflex::System::Win)

class MonitorInfo
{
public:

	MonitorInfo(HINSTANCE hinstance);

	Pair<Int, Float> GetMaxPixelDensity() const;

	Array< Pair<iRect, Float> > GetScreens() const;

	void Refresh();



private:

	struct Monitor
	{
		RECT rc; 
		Float raw_dpi;       
		Float effective_dpi; 
		Float effective_scale;
		UInt tier;
		Float adjustment; 
	};

	static BOOL CALLBACK EnumProc(HMONITOR hmon, HDC, LPRECT, LPARAM ctx);

	void AddMonitorX(HMONITOR hmon);



	HWND m_desktop_hwnd = GetDesktopWindow();

	void * m_get_monitor_dpi_fn = nullptr;

	Array <Monitor> m_monitors;

	UInt m_master_pixel_density = 1;

};

REFLEX_END

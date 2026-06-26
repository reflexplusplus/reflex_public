#pragma once

#include "../library.h"
#include "../ui/window.h"
#include "../../common/instance/plugin.h"





//
//Win::PluginWindow

REFLEX_NS(Reflex::System::Win)

class PluginWindow : public Common::PluginWindow <Window>
{
public:

	PluginWindow(Common::PluginInstance & plugin, bool resizable, HWND hostwindow);

	virtual ~PluginWindow();



protected:

	void SetStyleFlags(UInt32 flags) override {}

	void SetDisplayMode(WindowDisplay mode) override;

	void SetRect(const iRect & rect) override;

	void SendTop() override {}


	static LRESULT CALLBACK KeyboardHookProc(int code, WPARAM, LPARAM);

	static LRESULT CALLBACK MouseHookProc(int code, WPARAM, LPARAM);


	static inline UInt st_max_window_depth = 3;

};

REFLEX_END
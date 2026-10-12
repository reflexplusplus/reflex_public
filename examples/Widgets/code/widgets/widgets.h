#pragma once

#include "../[require].h"




namespace Widgets
{

	struct DemoDesc
	{
		CString::View id;
		WString::View name;
		bool stretch;
		FunctionPointer <Unretained <GLX::Object>()> ctr;
	};

	inline DemoDesc MakeDemo(CString::View id, WString::View name, bool stretch, FunctionPointer <Unretained <GLX::Object>()> ctr)
	{
		return { id, name, stretch, ctr };
	}


	extern const DemoDesc g_button, g_popup, g_rotary_slider, g_drag_edit, g_range_bar, g_zone;

	inline ConstRef <DemoDesc> g_demos[] = { g_button, g_popup, g_rotary_slider, g_drag_edit, g_range_bar, g_zone };

	constexpr UInt kNumDemo = GetArraySize(g_demos);

}

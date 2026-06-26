#pragma once

#include "../window.h"




//
//OSX::PluginWindow

REFLEX_NS(Reflex::System::OSX)

class PluginWindow : public Common::PluginWindow <Window>
{
public:

	PluginWindow(Common::PluginInstance & plugin, bool resizable, UInt32 style, bool ontop, void * host_window);



protected:

	void SetRect(const iRect & rect) override;

	void RequestResize(const iSize & size, Int32 wdelta, Int32 hdelta) override;

};

REFLEX_END

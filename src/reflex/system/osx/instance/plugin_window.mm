#include "plugin_window.h"




//
//plugin window

REFLEX_BEGIN_INTERNAL(Reflex::System::OSX)

class AuPluginWindow : public PluginWindow
{
public:

	using PluginWindow::PluginWindow;



private:

	void RequestResize(const iSize & size, Int32 wdelta, Int32 hdelta) override
	{
		(void)size;
		(void)wdelta;
		(void)hdelta;

		auto nssize = NSMakeSize(m_nsview->m_xywh.size.w, m_nsview->m_xywh.size.h);

		[m_nsview->m_host_view setFrameSize:nssize];
	}
};

REFLEX_END_INTERNAL

Reflex::System::OSX::PluginWindow::PluginWindow(Common::PluginInstance & plugin, bool resizable, UInt32 style, bool ontop, void * host_window)
	: Common::PluginWindow<Window>(plugin, resizable, style, ontop, host_window)
{
	[m_nsview setAutoresizingMask:0];
}

void Reflex::System::OSX::PluginWindow::SetRect(const iRect & rect)
{
	auto old_size = m_nsview->m_xywh.size;

	m_nsview->m_xywh = { {0, 0}, rect.size };

	auto nssize = NSMakeSize(rect.size.w, rect.size.h);

	[m_nsview setFrameSize:nssize];

	Int32 wdelta = rect.size.w - old_size.w;

	Int32 hdelta = rect.size.h - old_size.h;

	if (wdelta | hdelta)
	{
		RequestResize(m_nsview->m_xywh.size, wdelta, hdelta);
	}
}

void Reflex::System::OSX::PluginWindow::RequestResize(const iSize & size, Int32 wdelta, Int32 hdelta)
{
	(void)wdelta;
	(void)hdelta;

	plugin.OnSetViewSize(size);
}

Reflex::TRef <Reflex::System::Window> Reflex::System::Window::Create(UInt32 style, bool ontop, void * host_window)
{
	if (auto plugin = Common::PluginInstance::IsOpeningPluginWindow(host_window))
	{
		switch (plugin->session->format)
		{
		case Common::kPluginFormatVST3:
		case Common::kPluginFormatCLAP:
			return REFLEX_CREATE(OSX::PluginWindow, *plugin, True(style & kWindowStyleResizable), style, ontop, host_window);

		case Common::kPluginFormatAUv2:
			return REFLEX_CREATE(OSX::AuPluginWindow, *plugin, True(style & kWindowStyleResizable), style, ontop, host_window);

		default:
			return REFLEX_CREATE(OSX::PluginWindow, *plugin, false, style, ontop, host_window);
		}
	}
	else
	{
		return REFLEX_CREATE(OSX::Window, style, ontop, host_window);
	}
}

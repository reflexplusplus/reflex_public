#include "instance.h"




//
//

Reflex::CString::View Reflex::System::Common::g_plugin_host;

Reflex::Item<Reflex::System::App,false>::List Reflex::System::App::list;

Reflex::CString::View Reflex::System::AudioPlugin::GetPluginHost()
{
	return Common::g_plugin_host;
}

#include "../../common/instance/vst2.cpp"
#include "plugin_window.cpp"




//
//entry

BOOL WINAPI DllMain(HINSTANCE hinstance, DWORD msg, LPVOID)
{
	REFLEX_USE(Reflex);

	switch (msg)
	{
	case 1:
		Retain(System::Common::ThePluginSession::Acquire(hinstance, System::Common::kPluginFormatVST2));
		break;

	case 0:
		Release(*System::Common::ThePluginSession::Get());
		break;
	}

	return 1;
}

extern "C" REFLEX_EXPORT Reflex::VST2API::Plugin * VSTPluginMain(Reflex::VST2API::HostCallback vstcallback)
{
	return Reflex::System::Common::Vst2Instance::VSTPluginMainImpl(vstcallback);
}


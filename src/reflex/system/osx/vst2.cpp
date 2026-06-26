#include "globals.h"

#include "../common/instance/vst2.cpp"




//
//entry

extern "C" REFLEX_EXPORT Reflex::VST2API::Plugin * VSTPluginMain(Reflex::VST2API::HostCallback vstcallback)
{
	[[maybe_unused]] auto used = Reflex::System::Common::ThePluginSession::Acquire(nullptr, Reflex::System::Common::kPluginFormatVST2);

	return Reflex::System::Common::Vst2Instance::VSTPluginMainImpl(vstcallback);
}


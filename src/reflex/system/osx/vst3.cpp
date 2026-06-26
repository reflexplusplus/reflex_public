#include "globals.h"

#include "../common/instance/vst3.cpp"




//
//entry

REFLEX_BEGIN_INTERNAL(Reflex::System::Common)

iSize Vst3View::ToPixelUnits(const iSize & size)
{
	return size;
}

REFLEX_END_INTERNAL

extern "C" REFLEX_EXPORT bool bundleEntry(CFBundleRef ref)
{
	Reflex::Retain(Reflex::System::Common::TheVst3Session::Acquire(nullptr));

	return true;
}

extern "C" REFLEX_EXPORT bool bundleExit()
{
	Reflex::Release(*Reflex::System::Common::TheVst3Session::Get());

	return true;
}

extern "C" REFLEX_EXPORT void * GetPluginFactory()
{
	auto session = Reflex::System::Common::TheVst3Session::Get();

	Reflex::Retain(*session);

	return Reflex::Cast<Reflex::VST3API::IUnknown>(session).Adr();
}

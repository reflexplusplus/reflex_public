#include "../../common/instance/vst3.cpp"
#include "plugin_window.cpp"




//
//entry

REFLEX_BEGIN_INTERNAL(Reflex::System::Common)

iSize Vst3View::ToPixelUnits(const iSize & size)
{
	Int32 dpifactor = Win::globals->m_maxpixeldensity;

	return { size.w * dpifactor, size.h * dpifactor };
}

REFLEX_END_INTERNAL

REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

HINSTANCE gHinstance = 0;

REFLEX_END_INTERNAL

BOOL __stdcall DllMain(HINSTANCE hinstance, DWORD msg, LPVOID)
{
	if (msg == 1)
	{
		Reflex::System::Win::gHinstance = hinstance;
	}

	return 1;
}

extern "C" REFLEX_EXPORT bool __stdcall InitDll()
{
	Reflex::Retain(Reflex::System::Common::TheVst3Session::Acquire(Reflex::System::Win::gHinstance));

	return true;
}

extern "C" REFLEX_EXPORT bool __stdcall ExitDll()
{
	Reflex::Release(Reflex::System::Common::TheVst3Session::Get());

	return true;
}

extern "C" REFLEX_EXPORT void * __stdcall GetPluginFactory()
{
	auto session = Reflex::System::Common::TheVst3Session::Get();

	Reflex::Retain(session);

	return &Reflex::Cast<Reflex::VST3API::IUnknown>(*session);
}

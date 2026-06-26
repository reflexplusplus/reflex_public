#include "../../common/instance/aax.cpp"

#include "../../common/ext/aax/SDK/Interfaces/AAX_Init.h"

REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

HINSTANCE gHinstance = 0;

REFLEX_END_INTERNAL

BOOL __stdcall DllMain(HINSTANCE hinstance, DWORD msg, LPVOID)
{
	if (msg == 1)
	{
		Reflex::System::Win::gHinstance = hinstance;
		Reflex::Retain(Reflex::System::Common::ThePluginSession::Acquire(hinstance, Reflex::System::Common::kPluginFormatAAX));
	}
	else if (msg == 0)
	{
		Reflex::Release(*Reflex::System::Common::ThePluginSession::Get());
	}

	return 1;
}

#if defined(__GNUC__)
#define AAX_EXPORT extern "C" __attribute__ ((visibility ("default"))) ACFRESULT
#else
#define AAX_EXPORT extern "C" __declspec (dllexport) ACFRESULT __stdcall
#endif

AAX_EXPORT ACFRegisterPlugin (IACFUnknown * pUnkHost, IACFPluginDefinition ** ppPluginDefinition)
{
	return AAXRegisterPlugin(pUnkHost, ppPluginDefinition);
}

AAX_EXPORT ACFRegisterComponent (IACFUnknown * pUnkHost, acfUInt32 index, IACFComponentDefinition ** ppComponentDefinition)
{
	return AAXRegisterComponent(pUnkHost, index, ppComponentDefinition);
}

AAX_EXPORT ACFGetClassFactory (IACFUnknown * pUnkHost, const acfCLSID & clsid, const acfIID & iid, void ** ppOut)
{
	return AAXGetClassFactory(pUnkHost, clsid, iid, ppOut);
}

AAX_EXPORT ACFCanUnloadNow (IACFUnknown * pUnkHost)
{
	return AAXCanUnloadNow(pUnkHost);
}

AAX_EXPORT ACFStartup (IACFUnknown * pUnkHost)
{
	return AAXStartup(pUnkHost);
}

AAX_EXPORT ACFShutdown (IACFUnknown * pUnkHost)
{
	return AAXShutdown(pUnkHost);
}

AAX_EXPORT ACFGetSDKVersion (acfUInt64 * oSDKVersion)
{
	return AAXGetSDKVersion(oSDKVersion);
}

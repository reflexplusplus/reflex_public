#define REFLEX_LIBRARY
#define REFLEX_INCLUDE_UI false

#include "win/build.cpp"
#include "common/instance/library.cpp"

BOOL WINAPI DllMain(HINSTANCE hinstance, DWORD fdwReason, LPVOID lpvReserved)
{
	Reflex::System::Common::SetInstanceHandle(hinstance);

	//switch (fdwReason)
	//{
	//case DLL_PROCESS_ATTACH:
	//	Reflex::System::Common::DllInstance::st_hinstance = hinstance;
	//	break;

	//case DLL_PROCESS_DETACH:
	//	//if (lpvReserved) Reflex::System::Win::instance.Deinit();
	//	break;
	//}

	return true;
}

void Reflex::System::App::Quit()
{
}

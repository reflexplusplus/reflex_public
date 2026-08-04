#include "../library.h"




//
//entry

#ifdef REFLEX_SYSTEM_AUDIO
#include "../../common/instance/audioapp.cpp"
#include "asio.cpp"
#else
#include "../../common/instance/app.cpp"
#endif

INT WINAPI WinMain(HINSTANCE hinstance, HINSTANCE, LPSTR cmdline, INT)
{
	REFLEX_USE(Reflex);

	System::Common::SetInstanceHandle(hinstance);

	root_module.Init();

	Array <CString> args_store;
	Array <CString::View> args;

	if (cmdline[0] != char(0))
	{
		int argc = 0;
		LPWSTR * wargv = CommandLineToArgvW(GetCommandLineW(), &argc);

		if (auto narg = argc - 1)
		{
			args_store.Allocate(narg);

			auto pargview = Extend(args, narg).data;

			Array <UInt8> buffer;

			REFLEX_LOOP_PTR(wargv + 1, parg, narg)
			{
				buffer.Clear();

				const WChar * pwstring = *parg;

				Data::EncodeUTF8(buffer, pwstring);

				CString::View string_view = { Reinterpret<char>(buffer.GetData()), buffer.GetSize() };

				*pargview++ = args_store.Push<kAllocateNone>(string_view);
			}
		}
	}

#ifdef REFLEX_SYSTEM_AUDIO
	auto instance = System::Win::CreateASIO();
#else
	auto instance = New<System::Common::NonAudioApp>();
#endif

	if (instance->Initialise(args))
	{
		MSG msg = { 0, 0, 0, 0, 0, 0 };

		while (GetMessageW(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);

			DispatchMessage(&msg);
		}
	}

	instance->Deinitialise();

	root_module.Deinit();

	return 0;
}

const Reflex::System::EnvironmentType Reflex::System::kEnvironmentType = Reflex::System::kEnvironmentTypeDesktopApp;

void Reflex::System::App::Quit()
{
	PostQuitMessage(0);
}

#if REFLEX_INCLUDE_UI

#include "../ui/window.h"

Reflex::TRef <Reflex::System::Window> Reflex::System::Window::Create(UInt32 style, bool topmost, void * systemwindow)
{
	return REFLEX_CREATE(Win::Window, (HWND)systemwindow, style, topmost, false);
}

#else

#endif

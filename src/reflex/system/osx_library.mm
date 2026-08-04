#define REFLEX_MACOS_TARGET Library
#define REFLEX_LIBRARY

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvla"

#include "osx/build.mm"
#include "common/instance/library.cpp"

#pragma clang diagnostic pop

void Reflex::System::App::Quit()
{
	constexpr auto DoQuit = [](CFRunLoopTimerRef, void *)
	{
		[[NSApplication sharedApplication] terminate:nil];
	};

	auto timer = CFRunLoopTimerCreate(NULL, 0, 0, 0, 0, DoQuit, 0);

	CFRunLoopAddTimer(CFRunLoopGetMain(), timer, kCFRunLoopCommonModes);
}


#define REFLEX_LIBRARY

#include "win/build.cpp"
#include "common/instance/library.cpp"

#if REFLEX_INCLUDE_UI
Reflex::TRef <Reflex::System::Window> Reflex::System::Window::Create(UInt32 style, bool topmost, void * systemparent)
{
	return REFLEX_CREATE(Win::Window, (HWND)systemparent, style, topmost, false);
}
#endif

void Reflex::System::App::Quit()
{
	PostQuitMessage(0);
}

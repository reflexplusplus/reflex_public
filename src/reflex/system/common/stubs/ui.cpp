#include "[require].h"




//
//impl

Reflex::Int32 Reflex::System::GetMaxPixelDensity()
{
	return 1;
}

Reflex::Array <Reflex::System::iRect> Reflex::System::GetScreens()
{
	return { {} };
}

bool Reflex::System::IsDarkTheme()
{
	return false;
}

Reflex::Float Reflex::System::GetFontScale()
{
	return 1.0f;
}

Reflex::UInt8 Reflex::System::GetModifierKeys()
{
	return 0;
}

void Reflex::System::SetClipboard(const WString & string)
{
}

Reflex::WString Reflex::System::GetClipboard()
{
	return {};
}

Reflex::TRef <Reflex::System::Window> Reflex::System::Window::Create(UInt32 styleflags, bool topmost, void * systemparent)
{
	return {};
}

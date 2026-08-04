#pragma once

#include "reflex/glx.h"




//
//Addon API

namespace Reflex::GLX
{

	bool IsFullScreen(const WindowClient & window);

	bool ExitFullScreen(WindowClient & window);

	void ToggleFullScreen(WindowClient & window);


	System::RawBitmap CaptureWindow(const WindowClient & window);

}




//
//impl

inline bool Reflex::GLX::IsFullScreen(const WindowClient & window)
{
	return window.GetDisplayMode() == System::kWindowDisplayFullScreen;
}

inline bool Reflex::GLX::ExitFullScreen(WindowClient & window)
{
	if (IsFullScreen(window))
	{
		window.SetDisplayMode(System::kWindowDisplayWindowed);

		return true;
	}

	return false;
}

inline void Reflex::GLX::ToggleFullScreen(WindowClient & window)
{
	if (!ExitFullScreen(window))
	{
		window.SetDisplayMode(System::kWindowDisplayFullScreen);
	}
}

inline Reflex::System::RawBitmap Reflex::GLX::CaptureWindow(const WindowClient & window)
{
	auto system_window = window.owner;

	auto buffer = AutoRelease(system_window->CreateExportBitmapBuffer(1));

	system_window->ExportBitmap(buffer);

	return std::move(buffer->value);
}

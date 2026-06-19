#pragma once

#include "[require].h"




//
//Detail

namespace Reflex::Bootstrap::Detail
{

	TRef <GLX::WindowClient> CreateAppWindow(System::Window & window, GLX::Object & view);

	
	TRef <Data::PropertySet> CreateStylesheetOptions(bool dark_theme, Float font_scale, System::iSize screen_size);

	inline FunctionPointer <TRef<Data::PropertySet>()> g_create_stylesheet_options = []()
	{
		return CreateStylesheetOptions(GLX::kSystemTheme.a, GLX::kSystemTheme.b, System::GetScreens().GetFirst().size);
	};


	void SetStyle(GLX::Object & view, const WString::View & path, const ArrayView <Key32> & substyle, FunctionPointer <TRef <Data::PropertySet>()> create_options = g_create_stylesheet_options);


	GLX::Rect ConstrainRectToDisplay(const GLX::Rect & rect, GLX::Size min);

	void PublishAppView(System::App::Configuration & config, const Function <TRef<GLX::Object>(Object & instance_delegate)> & ctr);


	constexpr Key32 kViewGraphicsConfig = MakeKey32("view.graphics_config");


}

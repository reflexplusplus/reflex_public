#pragma once

#include "[require].h"




//
//Detail

namespace Reflex::Bootstrap::Detail
{

	[[nodiscard]] Unretained <GLX::WindowClient> CreateAppWindow(System::Window & window, GLX::Object & view);

	
	[[nodiscard]] Unretained <Data::PropertySet> CreateStylesheetOptions(bool dark_theme, Float font_scale, System::iSize screen_size, bool resizable);

	inline FunctionPointer <Unretained<Data::PropertySet>(const GLX::Object & client)> g_create_stylesheet_options = [](const GLX::Object & client) -> Unretained<Data::PropertySet>
	{
		return CreateStylesheetOptions(GLX::kSystemTheme.a, GLX::kSystemTheme.b, System::GetScreens().GetFirst().size, Data::GetBool(client, GLX::kresizable));
	};


	void SetStyle(GLX::Object & view, const WString::View & path, const ArrayView <Key32> & substyle, FunctionPointer <Unretained <Data::PropertySet>(const GLX::Object & client)> create_options = g_create_stylesheet_options);


	GLX::Rect ConstrainRectToDisplay(const GLX::Rect & rect, GLX::Size min);

	void PublishAppView(System::App::Configuration & config, const Function <Unretained<GLX::Object>(Object & instance_delegate)> & ctr);


	constexpr Key32 kViewGraphicsConfig = MakeKey32("view.graphics_config");


}

#pragma once

#include "[require].h"




//
//Detail

namespace Reflex::Bootstrap::Detail
{

	Reference <GLX::WindowClient> CreateWindowClient(System::Window & window, GLX::Object & content, File::PersistentPropertySet & session);

	TRef <Data::PropertySet> CreateStylesheetOptions(bool dark_theme, Float font_scale, System::iSize screen_size);

	TRef <Data::PropertySet> CreateDefaultStylesheetOptions(GLX::Object & view);

	void SetStyle(GLX::Object & view, const WString::View & path, const ArrayView <Key32> & substyle, FunctionPointer <TRef<Data::PropertySet>(GLX::Object & view)> create_options = &CreateDefaultStylesheetOptions);

	GLX::Rect ConstrainRectToDisplay(const GLX::Rect & rect, GLX::Size min);

	void PublishAppView(System::App::Configuration & config, const Function <TRef<GLX::Object>(Object & instance_delegate)> & ctr);


	constexpr Key32 kViewGraphicsConfig = MakeKey32("view.graphics_config");

}




//
//impl

inline Reflex::TRef <Reflex::Data::PropertySet> Reflex::Bootstrap::Detail::CreateDefaultStylesheetOptions(GLX::Object & view)
{
	return CreateStylesheetOptions(GLX::kSystemTheme.a, GLX::kSystemTheme.b, System::GetScreens().GetFirst().size);
}

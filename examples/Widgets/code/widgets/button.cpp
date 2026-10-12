#include "widgets.h"




//
//Button demo

const Widgets::DemoDesc Widgets::g_button = Widgets::MakeDemo("button", L"Button", false, []() -> Unretained <GLX::Object>
{
	auto button = New<GLX::Button>(L"Click me");

	GLX::BindClick(button, [button]()
	{
		GLX::Select(button, !GLX::IsSelected(button));
		
		output.Log("button clicked");
	});

	return button;
});

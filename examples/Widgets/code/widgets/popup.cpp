#include "widgets.h"




//
//Popup demo

namespace Widgets { namespace {

struct PopupDemo : GLX::Object
{
	PopupDemo()
	{
		GLX::AddFloat(*this, popup, GLX::kAlignmentCenter);
	}

	bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (auto menu = GLX::GetMenu(e); menu && src == popup)
		{
			GLX::BindClick(menu->AddItem(L"First action"), []() { output.Log("popup first action"); });
			GLX::BindClick(menu->AddItem(L"Second action"), []() { output.Log("popup second action"); });
			return true;
		}

		return Object::OnEvent(src, e);
	}

	void OnSetStyle(const GLX::Style & style) override
	{
		popup.SetStyle(style["Popup"]);
	}

	GLX::Popup popup;
};

} }

const Widgets::DemoDesc Widgets::g_popup = Widgets::MakeDemo("popup", L"Popup", false, []() -> Unretained <GLX::Object>
{
	return New<PopupDemo>();
});

#include "widgets.h"




//
//DragEdit demo

namespace Widgets { namespace {

struct DragEditDemo : GLX::Object
{
	DragEditDemo()
	{
		drag_edit.SetRange(-10.0f, 10.0f, 0.5f);
		drag_edit.SetDefault(0.0f);
		drag_edit.SetValue(0.0f);
		drag_edit.SetSensitivity(8.0f);
		GLX::AddFloat(*this, drag_edit, GLX::kAlignmentCenter);
	}

	bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (src == drag_edit && e.id == GLX::kTransaction)
		{
			output.Log("drag edit stage=", UInt(GLX::GetTransactionStage(e)), " value=", drag_edit.GetValue());
			return true;
		}

		return Object::OnEvent(src, e);
	}

	void OnSetStyle(const GLX::Style & style) override
	{
		drag_edit.SetStyle(style["DragEdit"]);
	}

	GLX::DragEdit drag_edit;
};

} }

const Widgets::DemoDesc Widgets::g_drag_edit = Widgets::MakeDemo("dragedit", L"DragEdit", false, []() -> Unretained <GLX::Object>
{
	return New<DragEditDemo>();
});

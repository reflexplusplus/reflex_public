#include "../../../../include/reflex_ext/glx/widgets/form.h"
#include "common.h"




//
//Form impl

Reflex::GLX::Form::Form(WillRetain <Object> body)
	: header(NoRetain(New<Label>())),
	body(NoRetain(body))
{
	REFLEX_ASSERT(body->GetAllocator());

	Retain(header);

	Retain(body);

	SetFlow(*this, kFlowY);

	AddInline(*this, header);

	AddInlineFlex(*this, body);
}

Reflex::GLX::Form::Form(const WString::View & label, WillRetain <Object> body)
	: Form(body)
{
	SetText(header, label);
}

Reflex::GLX::Form::~Form()
{
	Release(body);

	Release(header);
}

void Reflex::GLX::Form::OnSetStyle(const Style & style)
{
	GLX::Object::OnSetStyle(style);

	Detail::ApplySubStyle(header, style, kheader);

	Detail::ApplySubStyle(body, style, kbody);
}



//
//FormEx impl

Reflex::GLX::FormEx::FormEx(const WString::View & title, WillRetain <Object> body, WillRetain <Object> footer)
	: Form(title, body),
	footer(NoRetain(footer))
{
	Retain(footer);

	EnableMouse(header, false);

	EnableMouse(body, false);

	EnableMouse(footer, false);

	AddInline(*this, footer);
}

Reflex::GLX::FormEx::~FormEx()
{
	Release(footer);
}

void Reflex::GLX::FormEx::OnSetStyle(const Style & style)
{
	Form::OnSetStyle(style);

	footer->SetStyle(style[kfooter]);
}

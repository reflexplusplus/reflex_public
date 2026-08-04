#include "../../../../include/reflex_ext/glx/widgets/textarea.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

TRef <TextEditBehaviour> InitialiseTextArea(TextArea & textedit, bool multi_line, Key32 textid)
{
	auto content = textedit.GetContent();

	content->SetProperty(textid, New<Text>(multi_line));

	auto behaviour = TextEditBehaviour::Create(textid);

	content->SetDelegate(TextEditBehaviour::kTextEdit, behaviour);

	SetFlow(content, FlowFlags(multi_line));

	return behaviour;
}

REFLEX_END_INTERNAL

Reflex::GLX::TextArea::TextArea(bool multi_line, Key32 textid)
	: behaviour(InitialiseTextArea(*this, multi_line, textid))
{
	EnableMouseCapture(*this, true);

	EnableAutoFit(*this, false, !multi_line);

	InvertScrollAxis(!multi_line);
}

void Reflex::GLX::TextArea::OnSetProperty(Address address, Reflex::Object & object)
{
	REFLEX_ASSERT(address != MakeAddress<Text>(kvalue));

	GLX::Object::OnSetProperty(address, object);
}

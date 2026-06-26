#include "../../../../include/reflex_ext/glx/widgets/textarea.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

TRef <TextEditBehaviour> InitialiseTextArea(TextArea & textedit, bool multi_line, Key32 textid)
{
	auto content = New<Object>();

	textedit.SetContent(content);

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
	EnableAutoFit(*this, false, !multi_line);

	InvertScrollAxis(!multi_line);
}

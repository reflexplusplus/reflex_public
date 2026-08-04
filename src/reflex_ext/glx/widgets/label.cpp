#include "../../../../include/reflex_ext/glx/widgets/label.h"




//
//impl

Reflex::GLX::Label::Label(WString && value, Key32 id)
{
	SetProperty(id, New<Text>(std::move(value)));
}

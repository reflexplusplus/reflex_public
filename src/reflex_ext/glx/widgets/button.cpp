#include "../../../../include/reflex_ext/glx/widgets/button.h"




//
//implementation

Reflex::GLX::Button::Button()
{
	SetMouseCursor(kMouseCursorPointer);
}

Reflex::GLX::Button::Button(const WString::View & label)
{
	SetMouseCursor(kMouseCursorPointer);

	SetText(*this, label);
}

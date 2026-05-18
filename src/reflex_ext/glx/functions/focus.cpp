#include "../../../../include/reflex_ext/glx/functions/focus.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

REFLEX_END_INTERNAL

void Reflex::GLX::FocusBranch(Object & object)
{
	if (!Reflex::BranchContains<GLX::Object>(object, Core::desktop->GetFocus()))
	{
		object.Focus();
	}
}

void Reflex::GLX::RedirectFocus(Object & area, Object & object)
{
	auto itr = Core::desktop->GetFocus();

	while (itr)
	{
		if (itr == area)
		{
			//area must contain focus, so we can focus object

			object.Focus();

			return;
		}
		else if (itr == object)
		{
			//object already contains focus, so dont need to do anything

			return;
		}

		itr = itr->GetParent();
	}
}

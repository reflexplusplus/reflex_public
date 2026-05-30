#pragma once

#include "reflex/glx.h"




//
//Experimental API

namespace Reflex::GLX
{

	void SetOnAlign(Object & object, const Function <void(Object & object, bool isresponsive, Float & contenth, Detail::LayoutModel::AlignFn std_align)> & callback);

}

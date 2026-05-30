#pragma once

#include "standard.h"




//
//Experimental API

namespace Reflex::GLX
{

	void SetOnAlign(GLX::Object & object, const Function <void(GLX::Object & object, bool isresponsive, Float & contenth, GLX::Detail::LayoutModel::AlignFn std_align)> & callback);

}

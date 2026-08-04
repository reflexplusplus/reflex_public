#pragma once

#include "reflex/glx.h"




//
//Addon API

namespace Reflex::GLX
{

	class Button;

}




//
//Button

class Reflex::GLX::Button : public Object
{
public:

	REFLEX_OBJECT(GLX::Button, Object);



	//lifetime

	Button();

	Button(const WString::View & label);

};

REFLEX_SET_TRAIT(Reflex::GLX::Button, IsSingleThreadExclusive);

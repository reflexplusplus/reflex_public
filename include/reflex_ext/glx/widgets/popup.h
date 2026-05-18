#pragma once

#include "../behaviours/popup.h"
#include "menu.h"




//
//Addon API

namespace Reflex::GLX
{

	class Popup;

}




//
//Popup

class Reflex::GLX::Popup : public Object
{
public:

	REFLEX_OBJECT(GLX::Popup, Object);

	static constexpr Key32 kMenuOpen = Menu::kMenuOpen;	//forwarded from Menu



	//lifetime

	Popup();



	//components

	const TRef <PopupBehaviour> behaviour;

};

REFLEX_SET_TRAIT(Reflex::GLX::Popup, IsSingleThreadExclusive);




//
//impl

inline Reflex::GLX::Popup::Popup()
	: behaviour(PopupBehaviour::Create())
{
	SetDelegate(MakeKey32("Popup"), behaviour);
}
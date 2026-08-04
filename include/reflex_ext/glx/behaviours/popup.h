#pragma once

#include "../widgets/menu.h"




//
//Addon API

namespace Reflex::GLX
{

	class PopupBehaviour;

}




//
//PopupBehaviour

class Reflex::GLX::PopupBehaviour : public Object::Delegate
{
public:
	
	REFLEX_OBJECT(GLX::PopupBehaviour, Delegate);

	static TRef <PopupBehaviour> Create();


	virtual void SetConfig(FunctionPointer <TRef<GLX::Object>()> create_content = &Detail::CreateMenuContent, Key32 forward_event = Menu::kMenuOpen, Key32 content_style = kmenu) = 0;

	virtual GLX::Object * Open() = 0;
};

REFLEX_SET_TRAIT(Reflex::GLX::PopupBehaviour, IsSingleThreadExclusive);

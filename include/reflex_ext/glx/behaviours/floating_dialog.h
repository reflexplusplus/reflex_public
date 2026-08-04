#pragma once

#include "reflex/glx.h"




//
//Addon API

namespace Reflex::GLX
{

	class FloatingDialogBehaviour;

}




//
//FloatingDialogBehaviour

class Reflex::GLX::FloatingDialogBehaviour : public GLX::Object::Delegate
{
public:

	REFLEX_OBJECT(FloatingDialogBehaviour, Delegate);

	static constexpr Key32 kClassID = MakeKey32("FloatingDialog");


	FloatingDialogBehaviour();

	void Attach(GLX::Object & parent);

	void Detach();

	void Toggle(GLX::Object & parent);



private:

	virtual void OnAttachObject() override;

	virtual void OnDetachObject() override;

	virtual void OnAttachWindow() override;


	Reference <ResizeBehaviour> m_resize;

	Reference <MoveBehaviour> m_move;

	Reference <Object> m_attach_debounce_clock;
};

REFLEX_SET_TRAIT(Reflex::GLX::FloatingDialogBehaviour, IsSingleThreadExclusive);

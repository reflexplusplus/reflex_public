#pragma once

#include "button.h"




//
//Addon API

namespace Reflex::GLX
{

	class Paginator;

}




//
//Paginator

class Reflex::GLX::Paginator : public AbstractViewBar
{
public:

	//lifetime

	static TRef <AbstractViewBar> Create(AbstractViewPort&) { return REFLEX_CREATE(Paginator); }

	Paginator();



protected:

	virtual void SetFlow(UInt8 flowflags) override;

	virtual bool OnEvent(Object & src, Event & e) override;

	virtual void OnSetStyle(const GLX::Style & style) override;

	virtual void OnUpdate() override;

	virtual Trap OnMouseOver(Core::MouseAction mouseaction, UInt8 flags) override;


	bool m_show[2] = { false, false };

	Button m_prevnext[2];
};

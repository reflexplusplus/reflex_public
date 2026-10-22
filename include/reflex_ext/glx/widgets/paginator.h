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

	static const AbstractViewPort::ViewBarType kType;



protected:

	struct LayoutModel;

	Paginator();

	virtual bool OnEvent(Object & src, Event & e) override;

	virtual void OnSetStyle(const GLX::Style & style) override;

	virtual void OnUpdate() override;

	virtual Core::Trap OnPointerTender(Core::PointerAction action, const Core::Pointer & pointer, UInt8 flags) override;


	bool m_show[2] = { false, false };

	Button m_prevnext[2];
};

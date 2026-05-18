#pragma once

#include "form.h"




//
//Addon API

namespace Reflex::GLX
{

	class Accordion;

}




//
//Accordion

class Reflex::GLX::Accordion : public Form
{
public:

	REFLEX_OBJECT(GLX::Accordion, Form);

	REFLEX_GLX_EVENT_ID(AccordionOpen);
	REFLEX_GLX_EVENT_ID(AccordionClose);



	//lifetime

	Accordion(bool open = true);



	//show

	void Open(bool animate = true, bool reveal = true);

	void Close(bool animate = true);

	void Toggle(bool animate = true);

	bool IsOpen() const;



	//content

	void Clear();



protected:

	virtual bool OnEvent(Object & src, Event & e) override;


	UInt8 m_enterflags;
};

#pragma once

#include "accordion.h"




//
//declarations

namespace Reflex::GLX
{

	class AccordionGroup;

}




//
//AccordionGroup

class Reflex::GLX::AccordionGroup : public Object
{
public:

	REFLEX_OBJECT(GLX::AccordionGroup,Object);



	//lifetime

	AccordionGroup();



	//content

	Accordion & AddItem();



	//access

	void SetIndex(UInt idx, bool animate = true);

	Idx GetIndex() const;



protected:

	using Base = AccordionGroup;

	virtual void OnSetStyle(const Style & style);

	virtual bool OnEvent(Object & src, Event & e);



private:

	struct ComputedStyle;


	ConstReference <ComputedStyle> m_cstyle;

	bool m_allowclose;

};




//
//impl

struct Reflex::GLX::AccordionGroup::ComputedStyle : public Reflex::Object
{
	ComputedStyle(const Style & style);

	const Style & item;
};

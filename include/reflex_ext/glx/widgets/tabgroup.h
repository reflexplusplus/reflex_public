#pragma once

#include "form.h"
#include "button.h"
#include "selector.h"




//
//Primary API

namespace Reflex::GLX
{

	class TabGroup;

}




//
//TabGroup

class Reflex::GLX::TabGroup : public Form
{
public:

	REFLEX_OBJECT(GLX::TabGroup, Form);

	REFLEX_DECLARE_KEY32(tab);



	//lifetime

	TabGroup();



	//content

	void Clear();

	TRef <Object> AddPanel(const WString::View & label, TRef <Object> content, Key32 style_id = kcontent, Key32 tab_style_id = ktab);

	void RemovePanel(UInt idx);



	//access

	TRef <Selector> GetSelector() { return Cast<Selector>(body); }

	ConstTRef <Selector> GetSelector() const { return Cast<Selector>(body); }



protected:

	virtual void OnSetStyle(const Style & style) override;
	
	virtual void OnUpdate() override;	//forward to selector

	virtual bool OnEvent(Object & src, Event & e) override;



private:

	Orientation m_content_alignment;

	Array < Pair <Reference <Button>, Key32> > m_tabs;

};

REFLEX_SET_TRAIT(Reflex::GLX::TabGroup, IsSingleThreadExclusive);

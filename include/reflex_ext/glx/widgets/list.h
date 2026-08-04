#pragma once

#include "reflex/glx.h"




//
//Experimental API

namespace Reflex::GLX
{

	class List;

	using ListScroller = ScrollerOfType <List>;

}




//
//List

class Reflex::GLX::List : public AbstractList
{
public:

	REFLEX_OBJECT(GLX::List, AbstractList);

	REFLEX_GLX_EVENT_ID(ListReorder);		//@Transaction stage, @UInt from, @UInt to, @bool allow

	static constexpr Key32 kReorderState = "reorder";



	//lifetime

	List();



protected:

	virtual UInt OnGetSize() const override;

	virtual Idx OnGetIndex(Object & child) override;

	virtual void OnSelect(UInt idx) override;

	virtual void OnDeselect(UInt idx) override;

	virtual void OnEnumerateSelection(UInt start, UInt range, const Function <void(UInt idx, UInt n)> & callback) const override;

	virtual void OnReveal(UInt idx) override;


	virtual void OnUpdate() override;

	virtual bool OnEvent(Object & src, Event & e) override;

};

REFLEX_SET_TRAIT(Reflex::GLX::List, IsSingleThreadExclusive);

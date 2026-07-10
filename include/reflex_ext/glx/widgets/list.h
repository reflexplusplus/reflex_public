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



	//events

	REFLEX_GLX_EVENT_ID(ListReorder);		//@Transaction stage, @UInt from, @UInt to, @bool allow



	//states

	static constexpr Key32 kReorderState = "reorder";



	//lifetime

	List();



protected:

	UInt OnGetSize() const override;

	Idx OnGetIndex(Object & child) override;

	void OnSelect(UInt idx) override;

	void OnDeselect(UInt idx) override;

	void OnEnumerateSelection(UInt start, UInt range, const Function <void(UInt idx, UInt n)> & callback) const override;

	void OnReveal(UInt idx) override;


	void OnUpdate() override;

	bool OnEvent(Object & src, Event & e) override;

};

REFLEX_SET_TRAIT(Reflex::GLX::List, IsSingleThreadExclusive);

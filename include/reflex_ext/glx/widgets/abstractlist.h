#pragma once

#include "reflex/glx.h"




//
//Addon API

namespace Reflex::GLX
{

	class AbstractList;


	Idx GetListFocus(const AbstractList & list);

	Array <UInt> GetListSelection(const AbstractList & list);

	bool IsListSelected(const AbstractList & list, UInt idx);

	UInt CountListSelection(const AbstractList & list);

}




//
//AbstractList

class Reflex::GLX::AbstractList : public Object
{
public:

	//event ids

	REFLEX_GLX_EVENT_ID(ListSelect);			//UInt index, bool state, bool& allow
	REFLEX_GLX_EVENT_ID(ListLoad);				//UInt index
	REFLEX_GLX_EVENT_ID(ListStartDrag);			//UInt index
	REFLEX_GLX_EVENT_ID(ListRequestRemove);		//UInt index	//TODO replace with Remove(idx,bool&) which actually deletes with Exit



	//setup

	enum SelectionMode : UInt8
	{
		kSelectionModeSingle,
		kSelectionModeMulti,
		kSelectionModeMultiToggle
	};

	void SetSelectionMode(SelectionMode mode);


	
	//info
	
	UInt GetNumItem() const;



	//selection

	void SelectAll();

	void SelectNone();


	bool Select(UInt idx, bool multi = false);

	void Deselect(UInt idx);


	bool SelectNext(bool extend = false);

	bool SelectPrev(bool extend = false);


	void EnumerateSelection(UInt start, UInt range, const Function <void(UInt idx, UInt n)> & callback) const;


	
	//view
	
	void Reveal(UInt idx);



protected:

	//lifetime

	AbstractList(Detail::LayoutModelCtr layout);



	//callbacks

	virtual UInt OnGetSize() const = 0;

	virtual Idx OnGetIndex(Object & child) = 0;

	virtual void OnEnumerateSelection(UInt start, UInt range, const Function <void(UInt idx, UInt n)> & callback) const = 0;

	virtual void OnSelect(UInt idx) = 0;

	virtual void OnDeselect(UInt idx) = 0;

	virtual void OnReveal(UInt idx) = 0;


	virtual bool OnEvent(Object & src, Event & e) override;


	virtual void OnSetProperty(Address adr, Reflex::Object & object) override;



private:

	SelectionMode m_selection_mode;

	UInt8 m_multiselect_mask;

	UInt8 m_multiselect_flags;

};

REFLEX_SET_TRAIT(Reflex::GLX::AbstractList, IsSingleThreadExclusive);




//
//

REFLEX_INLINE Reflex::UInt Reflex::GLX::AbstractList::GetNumItem() const
{
	return OnGetSize();
}

REFLEX_INLINE void Reflex::GLX::AbstractList::EnumerateSelection(UInt start, UInt range, const Function <void(UInt idx, UInt n)> & callback) const
{
	OnEnumerateSelection(start, range, callback);
}

REFLEX_INLINE void Reflex::GLX::AbstractList::Reveal(UInt idx)
{
	OnReveal(idx);
}

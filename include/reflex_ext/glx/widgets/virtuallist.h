#pragma once

#include "abstractlist.h"




//
//Primary API

namespace Reflex::GLX
{

	class VirtualList;

	using VirtualListScroller = ScrollerOfType <VirtualList>;

	[[deprecated("use VirtualListScroller")]] typedef VirtualListScroller VectorScroller;

}




//
//VirtualList

class Reflex::GLX::VirtualList : public AbstractList
{
public:

	REFLEX_OBJECT(GLX::VirtualList, AbstractList);



	//types

	using Reference = Reflex::Reference <Object>;

	using PopulateCallback = Function <void(UInt start, ArrayRegion <Reference> items, const Style & style)>;



	//lifetime

	VirtualList();



	//setup

	void SetPopulateCallback(const PopulateCallback & onpopulate);



	//content

	void ClearItems();

	void SetNumItem(UInt n, bool force_refresh = false);


	void Rebuild();


	TRef <Object> GetItem(UInt idx);	//will return NULL if out of currently visible objects



protected:

	virtual void OnSetStyle(const Style & style) override;

	virtual bool OnEvent(Object & src, Event & e) override;



private:

	using PopulateCallbackProperty = ObjectOf <PopulateCallback>;


	virtual UInt OnGetSize() const override;

	virtual Idx OnGetIndex(Object & child) override;

	virtual void OnSelect(UInt idx) override;

	virtual void OnDeselect(UInt idx) override;

	virtual void OnEnumerateSelection(UInt start, UInt range, const Function <void(UInt idx, UInt n)> & callback) const override;

	virtual void OnReveal(UInt idx) override;


	template <bool Y> static void OnAccommodate(VirtualList & object, bool & isresponsive, Size & contentsize);

	template <bool Y> static void OnAlign(VirtualList & object, bool isresponsive, Float & contenth);


	ConstTRef <Style> m_item;

	Size m_itemsize;

	PopulateCallbackProperty * m_pcallback;


	UInt m_nitem;

	Pair <Int> m_visible;

	Array <Reference> m_items;

	Map <UInt> m_selection;

};

REFLEX_SET_TRAIT(Reflex::GLX::VirtualList, IsSingleThreadExclusive);

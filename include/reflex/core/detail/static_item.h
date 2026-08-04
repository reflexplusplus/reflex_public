#pragma once

#include "../meta/nothing.h"




//
//Detail

namespace Reflex::Detail
{

	template <class TYPE> class StaticItem;

}




//
//StaticItem

template <class TYPE>
class Reflex::Detail::StaticItem
{
public:

	struct Range
	{
		struct Itr;

		Itr begin();

		Itr end();
	};

	static inline Range range;



protected:

	StaticItem();

	StaticItem(NoValue hidden);



private:

	StaticItem * m_next;


	static inline StaticItem * st_first = nullptr;

};




//
//impl

template <class TYPE> REFLEX_INLINE Reflex::Detail::StaticItem<TYPE>::StaticItem()
	: m_next(0)
{
	REFLEX_STATIC_ASSERT(sizeof(StaticItem) == sizeof(void*));
	
	m_next = st_first;
	
	st_first = this;
}

template <class TYPE> REFLEX_INLINE Reflex::Detail::StaticItem<TYPE>::StaticItem(NoValue hidden)
	: m_next(0)
{
}

template <class TYPE>
struct Reflex::Detail::StaticItem<TYPE>::Range::Itr
{
	bool operator!=(Itr a) const { return ptr != a.ptr; }

	TYPE & operator*() const { return *static_cast<TYPE*>(ptr); }

	void operator++() { ptr = ptr->m_next; }

	StaticItem * ptr;
};

template <class TYPE> REFLEX_INLINE typename Reflex::Detail::StaticItem<TYPE>::Range::Itr Reflex::Detail::StaticItem<TYPE>::Range::begin() { return { static_cast<TYPE*>(st_first) }; }

template <class TYPE> REFLEX_INLINE typename Reflex::Detail::StaticItem<TYPE>::Range::Itr Reflex::Detail::StaticItem<TYPE>::Range::end() { return { nullptr }; }

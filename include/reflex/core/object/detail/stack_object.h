#pragma once

#include "../../detail/type_index.h"




//
//Detail

namespace Reflex::Detail
{

	#if REFLEX_DEBUG
	template <class TYPE> struct StaticObject;	//avoid non-heap debug warnings for static global objects
	#else
	template <class TYPE> using StaticObject = TYPE;
	#endif

}




//
//StaticObject

#if REFLEX_DEBUG

template <class TYPE>
struct Reflex::Detail::StaticObject : public TYPE
{
	template <class ... VARGS> StaticObject(VARGS && ... v);

	~StaticObject();

	virtual void OnDestruct() override {}
};

template <class TYPE> template <class ... VARGS> inline Reflex::Detail::StaticObject<TYPE>::StaticObject(VARGS && ... v)
	: TYPE(std::forward<VARGS>(v)...)
{
	REFLEX_STATIC_ASSERT(sizeof(StaticObject) == sizeof(TYPE));

	TYPE::SetOnHeap(*ToPointer<Allocator>(1));

	TYPE::GetActualRetainCount()++;
}

template <class TYPE> inline Reflex::Detail::StaticObject<TYPE>::~StaticObject()
{
	TYPE::SetOnStack();

	TYPE::GetActualRetainCount()--;
}

template <class TYPE>
struct Reflex::Detail::TypeIndex < Reflex::Detail::StaticObject <TYPE> >
{
	static inline const TypeID & value = TypeIndex<TYPE>::value;
};

#endif

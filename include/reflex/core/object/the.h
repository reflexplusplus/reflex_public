#pragma once

#include "../allocator.h"
#include "tref.h"
#include "traits.h"




//
//Secondary API

namespace Reflex
{

	template <class TYPE> class The;

}




//
//The

template <class TYPE>
class Reflex::The : public TYPE
{
public:

	REFLEX_STATIC_ASSERT_OBJECT_TYPE(TYPE);


	
	//lifetime

	template <class ... VARGS> static TRef <TYPE> Acquire(VARGS &&... v);



	//access

	static const bool & IsAwake();

	template <bool ASSUME = false> static TRef <TYPE> Get();



	//lifetime

	~The();



protected:

	template <class ... VARGS> The(VARGS &&... v);

	virtual void OnDestruct() override;



private:

	friend TYPE;

	friend Detail::Constructor <The>;


	static UInt8 st_bytes alignas(alignof(TYPE))[sizeof(TYPE)];

	static bool st_initalised;

};




//
//impl

template <class TYPE> Reflex::UInt8 Reflex::The<TYPE>::st_bytes alignas(alignof(TYPE))[];

template <class TYPE> bool Reflex::The<TYPE>::st_initalised = false;

template <class TYPE> template <class ... VARGS> REFLEX_INLINE Reflex::TRef <TYPE> Reflex::The<TYPE>::Acquire(VARGS &&... v)
{
	if (!st_initalised)
	{
		Detail::Constructor<The>::Construct(st_bytes, std::forward<VARGS>(v)...);
	}

	return *Reinterpret<The>(st_bytes);
}

template <class TYPE> REFLEX_INLINE const bool & Reflex::The<TYPE>::IsAwake()
{
	return st_initalised;
}

template <class TYPE> template <bool ASSUME> REFLEX_INLINE Reflex::TRef <TYPE> Reflex::The<TYPE>::Get()
{
	REFLEX_ASSERT(Copy(ASSUME) || st_initalised);

	return Reinterpret<The>(st_bytes);
}

template <class TYPE> template <class ... VARGS> inline Reflex::The<TYPE>::The(VARGS &&... v)
	: TYPE(std::forward<VARGS>(v)...)
{
	REFLEX_ASSERT(this == Get<true>().Adr());

	st_initalised = true;

	this->SetOnHeap(g_default_allocator);
}

template <class TYPE> inline Reflex::The<TYPE>::~The()
{
	st_initalised = false;
}

template <class TYPE> inline void Reflex::The<TYPE>::OnDestruct()
{
	Detail::Constructor<The>::Destruct(*this);
}

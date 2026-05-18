#pragma once

#include "../../detail/functions/allocate.h"
#include "../reference.h"
#include "../objectof.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE, class ... VARGS> TRef <TYPE> New(Allocator & allocator, VARGS && ... vargs);

	template <class TYPE, class ... VARGS> TRef <TYPE> New(VARGS && ... vargs);


	template <class TYPE, class ... VARGS> Reference <TYPE> Make(VARGS && ... vargs);


	template <class TYPE> inline TRef < ObjectOf <TYPE> > CreateObjectOf(const TYPE & value);

}




//
//Secondary API

#define REFLEX_CREATE_EX(allocator, TYPE, ...) Reflex::Detail::Constructor<TYPE>::SetOnHeap(new (Reflex::Detail::Allocate<TYPE>(allocator)) TYPE(__VA_ARGS__), allocator)

#define REFLEX_CREATE(TYPE, ...) REFLEX_CREATE_EX(Reflex::g_default_allocator, TYPE, __VA_ARGS__)




//
//impl

inline void * Reflex::Object::operator new(size_t size)
{
	return Object::operator new(size, g_default_allocator, AllocInfo());
}

inline void Reflex::Object::operator delete(void * adr)
{
	Object::operator delete(adr, g_default_allocator, AllocInfo());
}

inline void * Reflex::Object::operator new(size_t size, Allocator & allocator)
{
	return Object::operator new(size, allocator, AllocInfo());
}

inline void Reflex::Object::operator delete(void * adr, Allocator & allocator)
{
	Object::operator delete(adr, allocator, AllocInfo());
}

template <class TYPE> REFLEX_INLINE Reflex::TRef<TYPE>::TRef(NewObjectToken)
	: CommonReference<TYPE>(New<NonConstT<TYPE>>())
{
}

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> REFLEX_INLINE Reflex::Reference<TYPE,SAFE>::Reference(NewObjectToken)
	: Reference(New<NonConstT<TYPE>>())
{
}

template <class TYPE> template <class ... VARGS> REFLEX_INLINE Reflex::TRef <TYPE> Reflex::Detail::Constructor<TYPE>::New(Allocator & allocator, VARGS && ... vargs)
{
	if constexpr (kIsAbstract<TYPE>)
	{
		return TYPE::Create(std::forward<VARGS>(vargs)...);
	}
	else
	{
		return REFLEX_CREATE_EX(allocator, TYPE, std::forward<VARGS>(vargs)...);
	}
}

template <class TYPE, class ... VARGS> inline Reflex::TRef <TYPE> Reflex::New(VARGS && ... vargs)
{
	REFLEX_STATIC_ASSERT_OBJECT_TYPE(TYPE);

	return Detail::Constructor<TYPE>::New(g_default_allocator, std::forward<VARGS>(vargs)...);
}

template <class TYPE, class ... VARGS> inline Reflex::TRef <TYPE> Reflex::New(Allocator & allocator, VARGS && ... vargs)
{
	REFLEX_STATIC_ASSERT_OBJECT_TYPE(TYPE);

	return Detail::Constructor<TYPE>::New(allocator, std::forward<VARGS>(vargs)...);
}

template <class TYPE, class ... VARGS> inline Reflex::Reference <TYPE> Reflex::Make(VARGS && ... vargs)
{
	return New<TYPE>(std::forward<VARGS>(vargs)...);
}

template <class TYPE> inline Reflex::TRef < Reflex::ObjectOf <TYPE> > Reflex::CreateObjectOf(const TYPE & value)
{
	REFLEX_STATIC_ASSERT(!kIsObject<TYPE>);

	return New < ObjectOf <TYPE> >(value);
}

//by design not implemented, for heap use Object or ObjectOf

void * operator new(size_t size, Reflex::Allocator & allocator, const Reflex::AllocInfo & info);

void operator delete(void * ptr, Reflex::Allocator & allocator, const Reflex::AllocInfo & info);

void * operator new[](size_t size, Reflex::Allocator & allocator, const Reflex::AllocInfo & info);

void operator delete[](void * ptr, Reflex::Allocator & allocator, const Reflex::AllocInfo & info);

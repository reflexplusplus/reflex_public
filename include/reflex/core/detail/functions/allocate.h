#pragma once

#include "../../allocator.h"




//
//Detail

namespace Reflex::Detail
{

	template <class TYPE> class Constructor;


	template <class TYPE> TYPE * Allocate(Allocator & allocator);

	template <class TYPE> TYPE * Allocate(Allocator & allocator, const AllocInfo & info);

	template <class TYPE> TYPE * Allocate(Allocator & allocator, UInt n, const AllocInfo & info);

}




//
//Constructor

template <class TYPE>
class Reflex::Detail::Constructor
{
public:

	//raw

	template <class ...VARGS> static TYPE * Construct(void * adr_nonnull, VARGS && ... v);

	static void Destruct(const TYPE * adr_nonnull);

	static void Destruct(const TYPE & what);

	template <class ...VARGS> static void Reconstruct(TYPE & what, VARGS && ... v);



	//object

	template <class ... VARGS> [[nodiscard]] static TRef <TYPE> New(Allocator & allocator, VARGS && ... vargs);


	template <class ...VARGS> [[nodiscard]] static TYPE * CreateVariableSize(Allocator & allocator, UInt extra_size, VARGS &&... args);


	inline static TYPE * SetOnHeap(TYPE * what, Allocator & allocator) { REFLEX_ASSERT(what); what->GetBase()->SetOnHeap(allocator); return what; }

};




//
//impl

template <class TYPE> template <class ...VARGS> inline TYPE * Reflex::Detail::Constructor<TYPE>::Construct(void * adr_nonnull, VARGS && ... v)
{
	REFLEX_ASSERT(adr_nonnull);

	::new (adr_nonnull) TYPE(std::forward<VARGS>(v)...);

	return static_cast<TYPE*>(adr_nonnull);
}

template <class TYPE> inline void Reflex::Detail::Constructor<TYPE>::Destruct(const TYPE * adr_nonnull)
{
	REFLEX_ASSERT(adr_nonnull);

	adr_nonnull->~TYPE();
}

template <class TYPE> inline void Reflex::Detail::Constructor<TYPE>::Destruct(const TYPE & what)
{
	what.~TYPE();
}

template <class TYPE> template <class ...VARGS> inline void Reflex::Detail::Constructor<TYPE>::Reconstruct(TYPE & what, VARGS && ... v)
{
	Destruct(what);

	Construct(&what, std::forward<VARGS>(v)...);
}

template <class TYPE> REFLEX_INLINE TYPE * Reflex::Detail::Allocate(Allocator & allocator)
{
	return Allocate<TYPE>(allocator, REFLEX_ALLOCINFO(TYPE));
}

template <class TYPE> REFLEX_INLINE TYPE * Reflex::Detail::Allocate(Allocator & allocator, UInt n, const AllocInfo & info)
{
	REFLEX_ASSERT(((n * sizeof(TYPE)) / sizeof(TYPE)) == n);

	return Cast<TYPE>(allocator.Allocate(n * sizeof(TYPE), info));
}

template <class TYPE> REFLEX_INLINE TYPE * Reflex::Detail::Allocate(Allocator & allocator, const AllocInfo & info)
{
	return Cast<TYPE>(allocator.Allocate(sizeof(TYPE), info));
}

template <class TYPE> template <class ... VARGS> REFLEX_INLINE TYPE * Reflex::Detail::Constructor<TYPE>::CreateVariableSize(Allocator & allocator, UInt extra_size, VARGS &&... args)
{
	auto object = Cast<TYPE>(allocator.Allocate(sizeof(TYPE) + extra_size, REFLEX_ALLOCINFO(TYPE)));

	::new (object) TYPE(std::forward<VARGS>(args)...);

	return SetOnHeap(object, allocator);
}

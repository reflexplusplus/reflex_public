#pragma once

#include "functions/allocate.h"




//
//Detail

namespace Reflex::Detail
{

	template <class TYPE, UInt ALIGNMENT = alignof(TYPE), UInt SIZE = sizeof(TYPE)> struct Initialiser;

}




//
//Detail::Initialiser

template <class TYPE, Reflex::UInt ALIGNMENT, Reflex::UInt SIZE>
struct Reflex::Detail::Initialiser
{
	template <class ...VARGS> inline void Init(VARGS &&... v)
	{
		REFLEX_STATIC_ASSERT((ALIGNMENT >= alignof(TYPE)) && ((ALIGNMENT % alignof(TYPE)) == 0));
		REFLEX_STATIC_ASSERT(SIZE >= sizeof(TYPE));

		Constructor<TYPE>::Construct(m_raw, std::forward<VARGS>(v)...);
	}

	inline void Deinit()
	{
		Constructor<TYPE>::Destruct(Adr());
	}

	TYPE * Adr() { return std::launder(Reinterpret<TYPE>(m_raw)); }

	const TYPE * Adr() const { return std::launder(Reinterpret<TYPE>(m_raw)); }

	
	operator TYPE &() { return *Adr(); }

	operator const TYPE &() const { return *Adr(); }

	TYPE * operator->() { return Adr(); }

	const TYPE * operator->() const { return Adr(); }

	TYPE & operator*() { return *Adr(); }

	const TYPE & operator*() const { return *Adr(); }

	
	alignas(ALIGNMENT) UInt8 m_raw[SIZE];
};

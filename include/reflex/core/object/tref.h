#pragma once

#include "forward.h"
#include "common_reference.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE> class TRef;

	template <class TYPE> using ConstTRef = TRef <const TYPE>;

}




//
//TRef

template <class TYPE>
class Reflex::TRef : public CommonReference <TYPE>
{
public:

	//lifetime
	
	TRef();

	TRef(TYPE * non_null);

	TRef(TYPE & object);

	TRef(const TRef & tref) = default;

	template <class DERIVED> TRef(TRef <DERIVED> tref);

	template <ReferenceSafeFlags SAFE> TRef(Reference <TYPE,SAFE> & reference);

	template <ReferenceSafeFlags SAFE> TRef(const Reference <TYPE,SAFE> & reference);

	template <class DERIVED, ReferenceSafeFlags SAFE> TRef(Reference <DERIVED,SAFE> & reference);

	template <class DERIVED, ReferenceSafeFlags SAFE> TRef(const Reference <DERIVED,SAFE> & reference);

	template <class DERIVED, ReferenceSafeFlags SAFE> TRef(Reference <DERIVED, SAFE> && reference) = delete;		//unsafe semantic

	template <class DERIVED, ReferenceSafeFlags SAFE> TRef(const Reference <DERIVED, SAFE> && reference) = delete;	//unsafe semantic

	TRef(decltype(kNewObject));		//init with new instance

	TRef(decltype(kNullObject));	//init with null instance

	TRef(int) = delete;

	TRef(std::nullptr_t) = delete;

	constexpr TRef(NoValue special_zero_initalise);	//!advanced, special case for deferred init -> TRef MUST be logically non null



	//assign
	
	TRef & operator=(const TRef & tref) = default;



	//access
	
	TYPE * & Adr();

	TYPE * const & Adr() const;



	//special
	
	TRef <NonConstT<TYPE>> RemoveConst() const;



	//safety

	TRef <TYPE> * operator&() = delete;		//usage is typically a mistake, use GetAdr if actually needed

};

REFLEX_SET_TRAIT_TEMPLATED(TRef, IsBoolCastable);

template <class T> struct Reflex::IsReflexReference < Reflex::TRef <T> > { static constexpr bool value = true; static constexpr bool retaining = false; };




//
//impl

REFLEX_NS(Reflex)
template <class TYPE> inline void ByRef(TRef <TYPE> value);	//intentionally not implemented
REFLEX_END

REFLEX_STATIC_ASSERT(sizeof(Reflex::TRef<Reflex::Object>) == sizeof(void*));
REFLEX_STATIC_ASSERT(sizeof(Reflex::TRef<Reflex::Object>) == alignof(void*));

template <class TYPE> REFLEX_INLINE Reflex::TRef<TYPE>::TRef()
	: CommonReference<TYPE>(&Detail::GetNullInstance<NonConstT<TYPE>>())
{
}

template <class TYPE> REFLEX_INLINE Reflex::TRef<TYPE>::TRef(NullObjectToken)
	: CommonReference<TYPE>(&Detail::GetNullInstance<NonConstT<TYPE>>())
{
}

template <class TYPE> REFLEX_INLINE Reflex::TRef<TYPE>::TRef(TYPE * ptr)
	: CommonReference<TYPE>(ptr)
{
	REFLEX_ASSERT(ptr);
}

template <class TYPE> REFLEX_INLINE Reflex::TRef<TYPE>::TRef(TYPE & object)
	: CommonReference<TYPE>(&object)
{
}

template <class TYPE> REFLEX_INLINE constexpr Reflex::TRef<TYPE>::TRef(NoValue special_zero_initalise)
	: CommonReference<TYPE>(special_zero_initalise)
{
}

template <class TYPE> template <class DERIVED> REFLEX_INLINE Reflex::TRef<TYPE>::TRef(TRef <DERIVED> tref)
	: CommonReference<TYPE>(tref.Adr())
{
}

template <class TYPE> template <Reflex::ReferenceSafeFlags SAFE> REFLEX_INLINE Reflex::TRef<TYPE>::TRef(Reference <TYPE,SAFE> & ref)
	: CommonReference<TYPE>(ref.Adr())
{
}

template <class TYPE> template <Reflex::ReferenceSafeFlags SAFE> REFLEX_INLINE Reflex::TRef<TYPE>::TRef(const Reference <TYPE,SAFE> & ref)
	: CommonReference<TYPE>(ref.Adr())
{
}

template <class TYPE> template <class DERIVED, Reflex::ReferenceSafeFlags SAFE> REFLEX_INLINE Reflex::TRef<TYPE>::TRef(Reference <DERIVED,SAFE> & ref)
	: CommonReference<TYPE>(ref.Adr())
{
}

template <class TYPE> template <class DERIVED, Reflex::ReferenceSafeFlags SAFE> REFLEX_INLINE Reflex::TRef<TYPE>::TRef(const Reference <DERIVED,SAFE> & ref)
	: CommonReference<TYPE>(ref.Adr())
{
}

template <class TYPE> REFLEX_INLINE TYPE * & Reflex::TRef<TYPE>::Adr()
{ 
	return CommonReference<TYPE>::m_object; 
}

template <class TYPE> REFLEX_INLINE TYPE * const & Reflex::TRef<TYPE>::Adr() const
{
	return CommonReference<TYPE>::m_object;
}

template <class TYPE> REFLEX_INLINE Reflex::TRef <Reflex::NonConstT<TYPE>> Reflex::TRef<TYPE>::RemoveConst() const 
{ 
	return Reflex::RemoveConst(CommonReference<TYPE>::m_object); 
}

inline Reflex::TRef <Reflex::Object> Reflex::Object::GetBase() 
{ 
	return this; 
}

REFLEX_INLINE void Reflex::Object::SetProperty(Address address, TRef <Object> object)
{
	REFLEX_ASSERT(object.Adr() != this);

	OnSetProperty(address, object);
}

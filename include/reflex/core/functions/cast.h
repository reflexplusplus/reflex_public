#pragma once

#include "../preprocessor.h"
#include "../idx.h"	//TEMP




//
//Primary API

namespace Reflex
{

	template <class DERIVED, class TYPE> DERIVED * Cast(TYPE * type);

	template <class DERIVED, class TYPE> const DERIVED * Cast(const TYPE * type);


	template <class NEWTYPE, class TYPE> NEWTYPE & Reinterpret(TYPE & type);

	template <class NEWTYPE, class TYPE> const NEWTYPE & Reinterpret(const TYPE & type);


	template <class NEWTYPE, class TYPE> NEWTYPE * Reinterpret(TYPE * type);

	template <class NEWTYPE, class TYPE> const NEWTYPE * Reinterpret(const TYPE * type);


	template <class TYPE> UIntNative ToUIntNative(TYPE * type);

	template <class TYPE> TYPE * ToPointer(UIntNative value);


	template <class TYPE> TYPE & RemoveConst(const TYPE & type);

	template <class TYPE> TYPE * RemoveConst(const TYPE * type);

}




//
//impl

REFLEX_NS(Reflex::Detail)

template <class TYPE> void IncPointer(TYPE * & ptr, IntNative shift);

REFLEX_END

template <class DERIVED, class TYPE> REFLEX_INLINE DERIVED * Reflex::Cast(TYPE * type)
{
	return static_cast<DERIVED*>(type);
}

template <class DERIVED, class TYPE> REFLEX_INLINE const DERIVED * Reflex::Cast(const TYPE * type)
{
	return static_cast<const DERIVED*>(type);
}

template <class NEWTYPE, class TYPE> REFLEX_INLINE NEWTYPE & Reflex::Reinterpret(TYPE & type)
{
	REFLEX_STATIC_ASSERT(sizeof(NEWTYPE) <= sizeof(TYPE));
	REFLEX_STATIC_ASSERT((!IsType<TYPE, Idx>::value));

	return reinterpret_cast<NEWTYPE&>(type);
}

template <class NEWTYPE, class TYPE> REFLEX_INLINE const NEWTYPE & Reflex::Reinterpret(const TYPE & type)
{
	REFLEX_STATIC_ASSERT(sizeof(NEWTYPE) <= sizeof(TYPE));
	REFLEX_STATIC_ASSERT((!IsType<TYPE, Idx>::value));

	return reinterpret_cast<const NEWTYPE&>(type);
}

template <class NEWTYPE, class TYPE> REFLEX_INLINE NEWTYPE * Reflex::Reinterpret(TYPE * type)
{
	return reinterpret_cast<NEWTYPE*>(type);
}

template <class NEWTYPE, class TYPE> REFLEX_INLINE const NEWTYPE * Reflex::Reinterpret(const TYPE * type)
{
	return reinterpret_cast<const NEWTYPE*>(type);
}

template <class TYPE> REFLEX_INLINE Reflex::UIntNative Reflex::ToUIntNative(TYPE * type)
{
	return reinterpret_cast<UIntNative>(type);
}

template <class TYPE> REFLEX_INLINE TYPE * Reflex::ToPointer(UIntNative value)
{
	return reinterpret_cast<TYPE*>(value);
}

template <class TYPE> REFLEX_INLINE TYPE & Reflex::RemoveConst(const TYPE & type)
{
	return const_cast<TYPE&>(type);
}

template <class TYPE> REFLEX_INLINE TYPE * Reflex::RemoveConst(const TYPE * type)
{
	return const_cast<TYPE*>(type);
}

template <class TYPE> REFLEX_INLINE void Reflex::Detail::IncPointer(TYPE * & ptr, IntNative shift)
{
	ptr = ToPointer<TYPE>(ToUIntNative(ptr) + shift);
}

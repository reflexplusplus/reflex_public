#pragma once

#include "../../meta/traits.h"
#include "../../functions/memory.h"




//
//declarations

REFLEX_NS(Reflex::Detail)

template <class TYPE> void Initialise(TYPE & type);

REFLEX_END




//
//impl

REFLEX_NS(Reflex::Detail)

template <bool PRIMITIVE>
struct PrimitiveInitialiser
{
	template <class TYPE> REFLEX_INLINE static void Call(const TYPE & value) { }
};

template <>
struct PrimitiveInitialiser <true>
{
	template <class TYPE> REFLEX_INLINE static void Call(TYPE & value) { value = TYPE(); }
};

template <class TYPE, UInt SIZE>
struct RawInitialiser
{
	REFLEX_INLINE static void Call(TYPE & value) { MemClear(&value, sizeof(TYPE)); }
};

template <class TYPE>
struct RawInitialiser <TYPE,4>
{
	REFLEX_INLINE static void Call(TYPE & value) { Reinterpret<UInt32,TYPE>(value) = 0; }
};

template <class TYPE>
struct RawInitialiser <TYPE,8>
{
	REFLEX_INLINE static void Call(TYPE & value) { Reinterpret<UInt64,TYPE>(value) = 0; }
};

template <class TYPE> REFLEX_INLINE void Initialise(TYPE & value)
{
	if constexpr (kIsScalar<TYPE>)
	{
		using PrimitiveInitialiser = PrimitiveInitialiser < kIsScalar <TYPE> >;

		PrimitiveInitialiser::Call(value);
	}
	else if constexpr (kIsRawConstructible<TYPE>)
	{
		RawInitialiser<TYPE,sizeof(TYPE)>::Call(value);
	}
}

REFLEX_END

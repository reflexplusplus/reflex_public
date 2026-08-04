#pragma once

#include "../meta/traits.h"
#include "object.h"




//
//Secondary API

namespace Reflex
{

	template <class TYPE> struct IsSingleThreadExclusive { static constexpr bool value = false; };

	REFLEX_PUBLISH_TRAIT_VALUE(IsSingleThreadExclusive);


	template <class T> struct IsReflexReference { static constexpr bool value = false; static constexpr bool retaining = false; };

	REFLEX_PUBLISH_TRAIT_VALUE(IsReflexReference)


	template <class TYPE> struct SubIndexType { using Type = UInt32; };

}




//
//impl

template <class TYPE> REFLEX_INLINE Reflex::Address Reflex::MakeAddress(Key32 id) 
{
	REFLEX_STATIC_ASSERT_OBJECT_TYPE(TYPE);
	
	return { id, GetTypeID<TYPE>() };
}

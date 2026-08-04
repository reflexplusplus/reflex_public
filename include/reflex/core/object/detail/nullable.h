#pragma once

#include "../../meta.h"
#include "stack_object.h"




//
//Detail

#define REFLEX_EXTERN_NULL(TYPE) namespace Reflex::Detail { template <> struct ExternalNull <TYPE> { static TYPE & instance; }; }
#define REFLEX_INLINE_NULL(TYPE) REFLEX_STATIC_ASSERT(Reflex::kIsTrivial<TYPE>); namespace Reflex::Detail { template <> struct ExternalNull <TYPE> { inline static TYPE instance; }; }

REFLEX_NS(Reflex::Detail)

template <class TYPE> struct ExternalNull { /*inline static TYPE & instance;*/ };	//specialise to declare out-of-class null instance

template <typename TYPE, typename BASE> concept same_or_derived = std::derived_from <NonRefT<TYPE>, BASE>;

template <class TYPE> inline auto & GetNullInstance()
{
	REFLEX_STATIC_ASSERT(!kIsConst<TYPE>);
	REFLEX_STATIC_ASSERT(!kIsReference<TYPE>);

	if constexpr (requires { { TYPE::null } -> same_or_derived <TYPE>; })
	{
		return TYPE::null;
	}
	else if constexpr (requires { { ExternalNull<TYPE>::instance } -> same_or_derived <TYPE>; })
	{
		return ExternalNull<TYPE>::instance;
	}
	else
	{
		return Object::null;
	}
}

REFLEX_END

#define REFLEX_INSTANTIATE_EXTERN_NULL(TYPE, x) TYPE & Reflex::Detail::ExternalNull<TYPE>::instance = x

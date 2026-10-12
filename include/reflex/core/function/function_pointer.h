#pragma once

#include "detail/traits.h"
#include "../functions/memory.h"




//
//Primary API

namespace Reflex
{

	template <class SIG> using FunctionPointer = typename Detail::FunctionTraits<SIG>::PointerType;

}




//
//Detail

namespace Reflex::Detail
{

	template <class FUNCTION_POINTER_TYPE, class RETURN, class... ARGS> FUNCTION_POINTER_TYPE CastFunctionPointer(RETURN(*function)(ARGS...))
	{
		REFLEX_STATIC_ASSERT(sizeof(FUNCTION_POINTER_TYPE) == sizeof(function));
		FUNCTION_POINTER_TYPE result;
		MemCopy(&function, &result, sizeof(result));
		return result;
	}

}

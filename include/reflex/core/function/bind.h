#pragma once

#include "forward.h"




//
//Primary API

namespace Reflex
{

	template <class FN, class... VARGS> inline auto Bind(FN && fn, VARGS && ...vargs) { return std::bind(fn, std::forward<VARGS>(vargs)...); }

	template <class OBJECT, class FN, class... VARGS> inline auto BindMethod(OBJECT && object, FN && fn, VARGS &&... vargs) { return std::bind(fn, &Deref(object), std::forward<VARGS>(vargs)...); }

	template <class TYPE> inline auto ByRef(TYPE & value) { return std::reference_wrapper<TYPE>(value); }

}




//
//Detail

REFLEX_NS(Reflex::Detail)

template <class FUNCTION_POINTER_TYPE, class RETURN, class... ARGS> FUNCTION_POINTER_TYPE CastFunctionPointer(RETURN(*function)(ARGS...))
{
	static_assert(sizeof(FUNCTION_POINTER_TYPE) == sizeof(function));

	FUNCTION_POINTER_TYPE result;
	MemCopy(&function, &result, sizeof(result));
	return result;
}

REFLEX_END

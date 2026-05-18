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

#pragma once

#include "../[require].h"
#include <functional>




//
//Primary API

namespace Reflex
{

	constexpr auto _P1 = std::placeholders::_1;
	constexpr auto _P2 = std::placeholders::_2;
	constexpr auto _P3 = std::placeholders::_3;
	constexpr auto _P4 = std::placeholders::_4;
	constexpr auto _P5 = std::placeholders::_5;
	constexpr auto _P6 = std::placeholders::_6;


	template <class FN, class... VARGS> inline auto Bind(FN && fn, VARGS && ...vargs) { return std::bind(fn, std::forward<VARGS>(vargs)...); }

	template <class OBJECT, class FN, class... VARGS> inline auto BindMethod(OBJECT && object, FN && fn, VARGS &&... vargs) { return std::bind(fn, &Deref(object), std::forward<VARGS>(vargs)...); }


	template <class TYPE> inline auto ByRef(TYPE & value) { return std::reference_wrapper<TYPE>(value); }

}

#pragma once

#include "detail/binder.h"




//
//Primary API

namespace Reflex
{
	
	constexpr Detail::FunctionPlaceholder<0> _P1;
	constexpr Detail::FunctionPlaceholder<1> _P2;
	constexpr Detail::FunctionPlaceholder<2> _P3;
	constexpr Detail::FunctionPlaceholder<3> _P4;
	constexpr Detail::FunctionPlaceholder<4> _P5;
	constexpr Detail::FunctionPlaceholder<5> _P6;

	
	template <class FN, class... ARGS> auto Bind(FN && fn, ARGS && ... args);
   
	template <class CLASS, class FN, class... ARGS> auto BindMethod(CLASS && instance, FN && fn, ARGS && ... args);
	
	
	template <class TYPE> auto ByRef(TYPE & value);

}




//
//impl

template <class FN, class... ARGS> inline auto Reflex::Bind(FN && fn, ARGS && ... args)
{
	return Detail::FunctionBinder<std::decay_t<FN>, std::decay_t<ARGS>...>(std::forward<FN>(fn), std::forward<ARGS>(args)...);
}

template <class CLASS, class FN, class... ARGS> inline auto Reflex::BindMethod(CLASS && instance, FN && fn, ARGS && ... args)
{
	return Bind(std::forward<FN>(fn), &Deref(instance), std::forward<ARGS>(args)...);
}

template <class TYPE> inline auto Reflex::ByRef(TYPE & value)
{
	return Detail::FunctionReference<TYPE>{&value}; 
}

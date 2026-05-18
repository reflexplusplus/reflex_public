#pragma once

#include "../forward.h"
#include "nullreturn.h"




//
//Detail

REFLEX_NS(Reflex::Detail)

template <class SIG> struct FunctionTraits;

template <class RTN, class... ARGS>
struct FunctionTraits<RTN(ARGS...)>
{
	using Signature = RTN(ARGS...); 
	using PointerType = RTN(*)(ARGS...);

	static void Clear(Function <RTN(ARGS...)> & fn)
	{
		fn = [](ARGS...) -> RTN
		{
			return Detail::NullReturn<RTN>::Get();
		};
	}
};

template <class RTN, class... ARGS> 
struct FunctionTraits<std::function<RTN(ARGS...)>> : FunctionTraits<RTN(ARGS...)> 
{
};

template <class RTN, class... ARGS>
struct FunctionTraits<Function<RTN(ARGS...)>> : FunctionTraits<RTN(ARGS...)>
{
};

template <class T>
struct FunctionTraits : FunctionTraits<decltype(&T::operator())>
{
};


template <class CLASS, class SIG> struct MemberFunctionTraits;

template <class CLASS, class RTN, class... ARGS>
struct MemberFunctionTraits<CLASS, RTN(ARGS...)> : FunctionTraits<RTN(ARGS...)>
{
	using PointerType = RTN(CLASS::*)(ARGS...);
};

template <class CLASS, class RTN, class... ARGS>
struct FunctionTraits<RTN(CLASS::*)(ARGS...)> : FunctionTraits<RTN(ARGS...)>
{
};

template <class CLASS, class RTN, class... ARGS>
struct FunctionTraits<RTN(CLASS::*)(ARGS...) const> : FunctionTraits<RTN(ARGS...)>
{
};

template <class CLASS, class RTN, class... ARGS>
struct FunctionTraits<RTN(CLASS::*)(ARGS...) noexcept> : FunctionTraits<RTN(ARGS...)>
{
};

template <class CLASS, class RTN, class... ARGS>
struct FunctionTraits<RTN(CLASS::*)(ARGS...) const noexcept> : FunctionTraits<RTN(ARGS...)>
{
};

REFLEX_END

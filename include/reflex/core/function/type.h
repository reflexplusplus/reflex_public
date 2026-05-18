#pragma once

#include "detail/traits.h"




//
//Primary API

namespace Reflex
{

	template <class SIG> using FunctionPointer = typename Detail::FunctionTraits<SIG>::PointerType;

}

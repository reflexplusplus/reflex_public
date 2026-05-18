#pragma once

#include "detail/stack.h"




//
//Experimental API

namespace Reflex
{

	template <class TYPE> struct IsNonCircular { static constexpr bool value = !VM_ISOBJECT(TYPE); };

	REFLEX_PUBLISH_TRAIT_VALUE(IsNonCircular);


	template <class TYPE> struct IsThreadSafe { static constexpr bool value = !VM_ISOBJECT(TYPE); };

	REFLEX_PUBLISH_TRAIT_VALUE(IsThreadSafe);

}

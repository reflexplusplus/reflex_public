#pragma once

#include "../meta/traits.h"




//
//Secondary API

namespace Reflex
{

	template <class TYPE> struct IsNullTerminated { static constexpr bool value = false; };

	template <> struct IsNullTerminated <char> { static constexpr bool value = true; };

	template <> struct IsNullTerminated <WChar> { static constexpr bool value = true; };

	REFLEX_PUBLISH_TRAIT_VALUE(IsNullTerminated);

}

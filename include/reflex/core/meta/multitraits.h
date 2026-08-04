#pragma once

#include "../types.h"




//
//Secondary API

namespace Reflex
{

	template <template <typename> class TRAIT, class... TYPES>
	struct AndTraits 
	{
		static constexpr bool value = (TRAIT<TYPES>::value && ...);
	};

	template <template <typename> class TRAIT, class... TYPES>
	struct AddTraits 
	{
		static constexpr UInt value = (TRAIT<TYPES>::value + ...);
	};

}

#pragma once

#include "../types.h"




//
//Detail

namespace Reflex::Detail
{

	inline UInt32 g_type_index_counter = 0;

	template <class TYPE> struct TypeIndex { static inline const UInt32 value = ++g_type_index_counter; };

}

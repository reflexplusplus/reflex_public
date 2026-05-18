#pragma once

#include "defines.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE> class TRef;

	template <class TYPE, ReferenceSafeFlags SAFE = kReferenceDefaultSafeFlags> class Reference;

	class Allocator;

}




//
//Detail

namespace Reflex::Detail
{

	template <class TYPE> class Constructor;

}

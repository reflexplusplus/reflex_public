#pragma once

#include "detail/type_index.h"
#include "types.h"




//
//Primary API

namespace Reflex
{

	using TypeID = UInt32;

	template <class TYPE> TypeID GetTypeID();

}




//
//impl

#define REFLEX_TYPEID(TYPE) Reflex::Detail::TypeIndex<TYPE>::value

#define REFLEX_INSTANTIATE_TYPEID(...) namespace Reflex::Detail { [[maybe_unused]] inline const auto REFLEX_CONCATENATE(_kTypeID,__COUNTER__) = REFLEX_TYPEID(__VA_ARGS__); }

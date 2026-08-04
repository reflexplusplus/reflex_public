#pragma once

#include "detail/module.h"




//
//Secondary API

namespace Reflex
{

	extern Detail::Module root_module;		//no dependencies

}




//
//impl

template <class TYPE> Reflex::TypeID inline Reflex::GetTypeID()
{
	REFLEX_ASSERT(root_module.IsInitalised());

	return Detail::TypeIndex<TYPE>::value;
}


#pragma once

#include "../resourcepool.h"




//
//Primary API

namespace Reflex::File
{

	const ResourcePool::Token * Query(ResourcePool::Lock & lock, TypeID type, const WString::View & path);


	WString::View GetPath(const ResourcePool::Lock & lock, Address adr);

	WString::View GetResolvedPath(const ResourcePool::Lock & lock, Address adr);

}

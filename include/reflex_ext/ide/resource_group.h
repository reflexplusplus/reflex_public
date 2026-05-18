#pragma once

#include "[require].h"




//
//Primary API

namespace Reflex::IDE
{

	class ResourceGroup;

}




//
//IDE::ResourceGroup

class Reflex::IDE::ResourceGroup : public Object
{
public:

	static ResourceGroup & null;

	static TRef <ResourceGroup> Create(File::ResourcePool & resourcepool, Key32 uid, const WString::View & desc, const Function <void(ResourceGroup&)> & onreload);


	virtual void Clear() = 0;

	virtual void AddItem(Address adr, ConstTRef <Object> object) = 0;

	template <class TYPE> void AddItem(Key32 path, const TYPE & object) { AddItem(MakeAddress<TYPE>(path), object); }

	virtual void ForceRebuild(File::ResourcePool::Lock & lock) = 0;
};

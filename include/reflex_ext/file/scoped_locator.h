#pragma once

#include "[require].h"




//
//Primary API

namespace Reflex::File
{

	template <class TYPE> class ScopedLocator;

}




//
//ScopedLocator

template <class TYPE>
class Reflex::File::ScopedLocator
{
public:

	REFLEX_NONCOPYABLE(ScopedLocator);

	template <class ... VARGS> ScopedLocator(VirtualFileSystem::Lock & lock, VARGS && ... vargs)
		: locator(New<TYPE>(std::forward<VARGS>(vargs)...)),
		lock(lock)
	{
		lock.Attach(locator);
	}

	ScopedLocator(ScopedLocator && rhs)
		: locator(std::move(rhs.locator)),
		lock(rhs.lock)
	{
	}

	~ScopedLocator()
	{
		if (locator->GetRetainCount() == 2)
		{
			lock.Detach(locator);
		}
		else
		{
			REFLEX_ASSERT(false);
		}
	}

	VirtualFileSystem::Lock & lock;

	const Reference <TYPE> locator;

};

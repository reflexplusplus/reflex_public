#pragma once

#include "defines.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE> class TRef;

	//Generic non-owning, non-null references for values which are not Objects.
	template <class TYPE> using Ref = TRef <TYPE>;
	template <class TYPE> using ConstRef = Ref <const TYPE>;

	template <class TYPE, ReferenceSafeFlags SAFE = kReferenceDefaultSafeFlags> class Reference;


	//Strong owning reference which retains the object.
	template <class TYPE, ReferenceSafeFlags SAFE = kReferenceDefaultSafeFlags> using Retained = Reference <TYPE,SAFE>;
	template <class TYPE, ReferenceSafeFlags SAFE = kReferenceDefaultSafeFlags> using ConstRetained = Reference <const TYPE, SAFE>;

	//Explicit non-owned state, typically used for factory returns.
	template <class TYPE> using Unretained = TRef <TYPE>;

	//Non-owning scoped view of an object retained elsewhere.
	template <class TYPE> using AlreadyRetained = TRef <TYPE>;
	template <class TYPE> using ConstAlreadyRetained = AlreadyRetained <const TYPE>;

	//Function argument whose receiver promises to retain the object.
	template <class TYPE> using WillRetain = TRef <TYPE>;
	template <class TYPE> using ConstWillRetain = WillRetain <const TYPE>;

	class Allocator;

}




//
//Detail

namespace Reflex::Detail
{

	template <class TYPE> class Constructor;

}

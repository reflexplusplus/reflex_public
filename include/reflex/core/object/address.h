#pragma once

#include "../typeid.h"
#include "../key.h"
#include "../functions/cast.h"




//
//Primary API

namespace Reflex
{

	struct Address
	{
		bool operator==(const Address & b) const { return Reinterpret<UInt64>(*this) == Reinterpret<UInt64>(b); }
		bool operator!=(const Address & b) const { return Reinterpret<UInt64>(*this) != Reinterpret<UInt64>(b); }
		bool operator<(const Address & b) const { return Reinterpret<UInt64>(*this) < Reinterpret<UInt64>(b); }

		Key32 id;
		TypeID type_id;
	};

	template <class TYPE> Address MakeAddress(Key32 id);

}

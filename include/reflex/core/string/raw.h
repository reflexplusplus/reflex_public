#pragma once

#include "../types.h"




//
//Primary API

namespace Reflex
{

	template <class CHARACTER> constexpr UInt RawStringLength(const CHARACTER * from);

	template <class CHARACTER> constexpr UInt RawStringLength(const CHARACTER * from, UInt capacity);

	template <class FROM_CHARACTER, class TO_CHARACTER> void RawStringCopy(const FROM_CHARACTER * from, TO_CHARACTER * to, UInt capacity);

}




//
//impl

template <class CHAR> inline constexpr Reflex::UInt Reflex::RawStringLength(const CHAR * string)
{
	UInt length = 0;

	while (*string++ != 0) ++length;

	return length;
}

template <class CHAR> inline constexpr Reflex::UInt Reflex::RawStringLength(const CHAR * string, UInt capacity)
{
	UInt length = 0;

	while (length < capacity && *string != 0)
	{
		++string;
		++length;
	}

	return length;
}

template <class FROM, class TO> REFLEX_INLINE void Reflex::RawStringCopy(const FROM * from, TO * to, UInt capacity)
{
	if (capacity > 1)
	{
		auto end = to + capacity - 1;

		FROM c = 0;

		while ((c = *from++) && to < end)
		{
			*to++ = TO(c);
		}

		*to = 0;
	}
	else if (capacity)
	{
		to[0] = 0;
	}
}

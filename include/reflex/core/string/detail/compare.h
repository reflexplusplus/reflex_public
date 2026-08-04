#pragma once

#include "../../types.h"




//
//Detail

REFLEX_NS(Reflex::Detail)

template <bool CASE_INSENSITIVE, class CHARACTER> bool StringEquals(const CHARACTER * a, UInt a_length, const CHARACTER * b, UInt b_length);

template <bool CASE_INSENSITIVE, class CHARACTER> bool StringLessThan(const CHARACTER * a, UInt a_length, const CHARACTER * b, UInt b_length);

REFLEX_END




//
//impl

REFLEX_NS(Reflex)
char Lowercase(char character);
char16_t Lowercase(char16_t character);
WChar Lowercase(WChar character);
REFLEX_END

template <bool CASE_INSENSITIVE, class CHARACTER> inline bool Reflex::Detail::StringEquals(const CHARACTER * a, UInt a_length, const CHARACTER * b, UInt b_length)
{
	if (a_length == b_length)
	{
		if constexpr (CASE_INSENSITIVE)
		{
			auto pb = b;

			REFLEX_LOOP_PTR(a, pa, a_length)
			{
				if (Lowercase(*pa) != Lowercase(*pb++))
				{
					return false;
				}
			}

			return true;
		}
		else
		{
			return MemCompare(a, b, a_length * sizeof(CHARACTER));
		}
	}
	else
	{
		return false;
	}
}

template <bool CASE_INSENSITIVE, class CHARACTER> inline bool Reflex::Detail::StringLessThan(const CHARACTER * a, UInt a_length, const CHARACTER * b, UInt b_length)
{
	UInt shortest = b_length;

	bool shorter = false;

	if (a_length < shortest)
	{
		shorter = true;
		shortest = a_length;
	}

	auto pb = b;

	REFLEX_LOOP_PTR(a, pa, shortest)
	{
		auto ta = *pa;
		auto tb = *pb++;

		if constexpr (CASE_INSENSITIVE)
		{
			ta = Lowercase(ta);
			tb = Lowercase(tb);
		}

		if (ta == tb)
		{
			continue;
		}

		return ta < tb;
	}

	return shorter;
}

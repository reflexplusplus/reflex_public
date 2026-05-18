#pragma once

#include "../../meta/auxtypes.h"




//
//impl

REFLEX_NS(Reflex::Detail)

template <class RTN>
struct NullReturn
{
	using type = RTN;

	static type Get()
	{
		return {};
	}
};

template <>
struct NullReturn <void>
{
	using type = void;

	static void Get() {}
};

template <class RTN>
struct NullReturn <RTN*>
{
	using type = RTN *;

	static RTN * Get() { return nullptr; }
};

template <class RTN>
struct NullReturn <RTN&>
{
	using type = RTN &;

	//intentionlly undefined
};

template <class RTN>
struct NullReturn <RTN&&>
{
	using type = RTN &&;

	//intentionlly undefined
};

REFLEX_END

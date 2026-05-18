#pragma once

#include "../assert.h"




//
//Primary API

namespace Reflex
{

	enum AllocatePolicy
	{
		kAllocateExact,
		kAllocateOver,
		kAllocateNone,	//UNCHECKED!
	};


	template <class TYPE> class Array;

	template <class TYPE> class ArrayRegion;

	template <class TYPE> using ArrayView = ArrayRegion <const TYPE>;

}

#pragma once

#include "../types.h"




//
//Primary API

namespace Reflex
{

	using ReferenceSafeFlags = UInt8;

	constexpr UInt8 kReferenceNullable = 1;
	constexpr UInt8 kReferenceOnHeap = 2;

	constexpr UInt8 kReferenceUnsafe = 0;
	constexpr UInt8 kReferenceDefaultSafeFlags = kReferenceNullable;
	constexpr UInt8 kReferenceStrictSafeFlags = kReferenceNullable | kReferenceOnHeap;


	enum NewObjectToken { kNewObject };

	enum NullObjectToken { kNullObject };

}


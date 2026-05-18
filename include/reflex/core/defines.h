#pragma once

#include "types.h"




//
//Primary API

namespace Reflex
{

	constexpr UInt8 kMaxUInt8 = 0xff;

	constexpr UInt16 kMaxUInt16 = 0xffff;

	constexpr UInt32 kMaxUInt32 = 0xfffffffful;

	constexpr UInt64 kMaxUInt64 = 0xffffffffffffffffull;

	constexpr UIntNative kMaxUIntNative = ~(UIntNative(0));


	constexpr Int8 kMinInt8 = -0x80;

	constexpr Int8 kMaxInt8 = 0x7f;

	constexpr Int16 kMinInt16 = -0x8000;

	constexpr Int16 kMaxInt16 = 0x7fff;

	constexpr Int32 kMinInt32 = 0x80000000l;

	constexpr Int32 kMaxInt32 = 0x7fffffffl;

	constexpr Int64 kMinInt64 = 0x8000000000000000ll;

	constexpr Int64 kMaxInt64 = 0x7fffffffffffffffll;


	constexpr Float64 kPi = 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170;

	constexpr Float64 k2Pi = kPi * 2.0;


	constexpr Float32 kPif = Float32(kPi);

	constexpr Float32 k2Pif = Float32(k2Pi);

}

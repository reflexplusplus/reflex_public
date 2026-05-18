#pragma once

#include "../types.h"
#include "../idx.h"
#include "cast.h"




//
//Primary API

namespace Reflex
{

	constexpr UInt32 MakeBit(UInt32 idx, bool value = true);


	template <class UINT, class IDX> constexpr bool BitCheck(UINT flags, IDX idx);

	template <class UINT, class IDX> constexpr UINT BitClear(UINT flags, IDX idx);

	template <class UINT, class IDX> constexpr UINT BitSet(UINT flags, IDX idx);

	template <class UINT, class IDX> constexpr UINT BitSet(UINT flags, IDX idx, bool value);

	template <class UINT, class IDX> constexpr UINT BitFlip(UINT flags, IDX idx);


	template <class UINT> UINT BitFill(UInt n);


	template <class TYPE> TYPE BitReverse(TYPE value);


	UInt8 BitCount(UInt8 flags);

	UInt8 BitCount(UInt16 flags);

	UInt BitCount(UInt32 flags);

	UInt BitCount(UInt64 flags);


	Idx GetFirstBit(UInt8 value);

	Idx GetFirstBit(UInt16 value);

	Idx GetFirstBit(UInt32 value);

	Idx GetFirstBit(UInt64 value);


	Idx GetLastBit(UInt8 value);

	Idx GetLastBit(UInt16 value);

	Idx GetLastBit(UInt32 value);

	Idx GetLastBit(UInt64 value);


	UInt8 MakeBits(bool bit0, bool bit1);

	UInt8 MakeBits(bool bit0, bool bit1, bool bit2, bool bit3 = false);

	UInt8 MakeBits(bool bit0, bool bit1, bool bit2, bool bit3, bool bit4, bool bit5 = false, bool bit6 = false, bool bit7 = false);

}




//
//impl

REFLEX_NS(Reflex::Detail)

template <UInt SIZE> struct BitReverser {};

template <> struct BitReverser <1> { static UInt8 Call(void * value); };

template <> struct BitReverser <2> { static UInt16 Call(void * value); };

template <> struct BitReverser <4> { static UInt32 Call(void * value); };


extern const UInt8 kBitCount[256];

extern const Int8 kBitFirst[256];

extern const Int8 kBitLast[256];

REFLEX_END




//
//

REFLEX_INLINE Reflex::UInt8 Reflex::MakeBits(bool bit0, bool bit1)
{
	return bit0 + (bit1 * 2);
}

REFLEX_INLINE Reflex::UInt8 Reflex::MakeBits(bool bit0, bool bit1, bool bit2, bool bit3)
{
	return bit0 + (bit1 * 2) + (bit2 * 4) + (bit3 * 8);
}

REFLEX_INLINE Reflex::UInt8 Reflex::MakeBits(bool bit0, bool bit1, bool bit2, bool bit3, bool bit4, bool bit5, bool bit6, bool bit7)
{
	return bit0 + (bit1 * 2) + (bit2 * 4) + (bit3 * 8) + (bit4 * 16) + (bit5 * 32) + (bit6 * 64) + (bit7 * 128);
}

template <class TYPE, class IDX> REFLEX_INLINE constexpr bool Reflex::BitCheck(TYPE flags, IDX idx)
{
	return ((TYPE(1) << idx) & flags) != 0;
}

template <class UINT, class IDX> REFLEX_INLINE constexpr UINT Reflex::BitClear(UINT flags, IDX idx)
{
	return flags & ~(UINT(1) << idx);
}

REFLEX_INLINE constexpr Reflex::UInt32 Reflex::MakeBit(UInt32 idx, bool value)
{
	return value ? (UInt32(1) << idx) : false;
}

template <class UINT, class IDX> REFLEX_INLINE constexpr UINT Reflex::BitSet(UINT flags, IDX idx)
{
	return flags | (UINT(1) << idx);
}

template <class UINT, class IDX> REFLEX_INLINE constexpr UINT Reflex::BitFlip(UINT flags, IDX idx)
{
	return flags ^ (UINT(1) << idx);
}

template <class UINT, class IDX> REFLEX_INLINE constexpr UINT Reflex::BitSet(UINT flags, IDX idx, bool value)
{
	UINT t = (UINT(1) << idx);

	return (flags & ~t) + (t * value);
}

template <class UINT> REFLEX_INLINE UINT Reflex::BitFill(UInt n)
{
	return ((1 << UINT(n)) - 1);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::BitReverse(TYPE value)
{
	return Reinterpret<TYPE>(Detail::BitReverser<sizeof(TYPE)>::Call(&value));
}

REFLEX_INLINE Reflex::UInt8 Reflex::BitCount(UInt8 value)
{
	return Detail::kBitCount[value];
}

REFLEX_INLINE Reflex::UInt8 Reflex::BitCount(UInt16 value)
{
	const UInt8 * values = Reinterpret<UInt8>(&value);

	return Detail::kBitCount[values[0]] + Detail::kBitCount[values[1]];
}

REFLEX_INLINE Reflex::UInt32 Reflex::BitCount(UInt32 value)
{
	value = value - ((value >> 1) & 0x55555555);				// reuse input as temporary

	value = (value & 0x33333333) + ((value >> 2) & 0x33333333);	// temp

	//return ((value + (value >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
	return (((value + (value >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
}

REFLEX_INLINE Reflex::UInt Reflex::BitCount(UInt64 value)
{
	const UInt * values = Reinterpret<UInt>(&value);

	return BitCount(values[0]) + BitCount(values[1]);
}

REFLEX_INLINE Reflex::Idx Reflex::GetFirstBit(UInt8 value)
{
	return Detail::kBitFirst[value];
}

REFLEX_INLINE Reflex::Idx Reflex::GetFirstBit(UInt16 value)
{
	auto values = Reinterpret<UInt8>(&value);

	UInt8 value0 = *values;

	return value0 ? Detail::kBitFirst[value0] : (Detail::kBitFirst[values[1]] + 8);
}

REFLEX_INLINE Reflex::Idx Reflex::GetFirstBit(UInt32 value)
{
	auto values = Reinterpret<UInt8>(&value);

	REFLEX_LOOP(idx, 4)
	{
		UInt8 byte = values[idx];

		if (byte) return (idx * 8) + Detail::kBitFirst[byte];
	}

	return kMaxUInt32;
}

REFLEX_INLINE Reflex::Idx Reflex::GetFirstBit(UInt64 value)
{
	auto values = Reinterpret<UInt8>(&value);

	REFLEX_LOOP(idx, 8)
	{
		UInt8 byte = values[idx];

		if (byte) return Detail::kBitFirst[byte] + (idx * 8);
	}

	return kMaxUInt32;
}

REFLEX_INLINE Reflex::Idx Reflex::GetLastBit(UInt8 value)
{
	return Detail::kBitLast[value];
}

REFLEX_INLINE Reflex::Idx Reflex::GetLastBit(UInt16 value)
{
	auto values = Reinterpret<UInt8>(&value);

	UInt8 value1 = values[1];

	return (value1 != 0) ? (Detail::kBitLast[value1] + 8): Detail::kBitLast[values[0]];
}

REFLEX_INLINE Reflex::Idx Reflex::GetLastBit(UInt32 value)
{
	const UInt8 * values = Reinterpret<UInt8>(&value);

	REFLEX_RLOOP(idx, 4)
	{
		UInt8 byte = values[idx];

		if (byte) return Detail::kBitLast[byte] + (idx * 8);
	}

	return kMaxUInt32;
}

REFLEX_INLINE Reflex::Idx Reflex::GetLastBit(UInt64 value)
{
	auto values = Reinterpret<UInt8>(&value);

	REFLEX_RLOOP(idx, 8)
	{
		UInt8 byte = values[idx];

		if (byte) return (idx * 8) + Detail::kBitLast[byte];
	}

	return kMaxUInt32;
}




//
//detail

REFLEX_NS(Reflex::Detail)

REFLEX_INLINE UInt8 BitReverser<1>::Call(void * pvalue)
{
	UInt8 b = *Cast<UInt8>(pvalue);

	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;

	return b;
}

REFLEX_INLINE UInt16 BitReverser<2>::Call(void * pvalue)
{
	UInt32 value = *Cast<UInt16>(pvalue);

	return UInt16((value >> 8) | (value << 8));
}

REFLEX_INLINE UInt BitReverser<4>::Call(void * pvalue)
{
	UInt32 value = *Cast<UInt32>(pvalue);

	return (value >> 24) | ((value & 0x00ff0000) >> 8) | ((value & 0x0000ff00) << 8) | (value << 24);
}

REFLEX_END

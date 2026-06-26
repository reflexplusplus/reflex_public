#include "../../../include/reflex/data/functions/hash.h"




//
//implementation

#define SHA1_MACRO(func,val) { const UInt t = SHA1_Rol(a, 5) + (func) + e + val + w[round]; e = d; d = c; c = SHA1_Rol(b, 30); b = a; a = t; } ++round;

REFLEX_BEGIN_INTERNAL(Reflex::Data)

REFLEX_INLINE UInt SHA1_Rol(UInt value, UInt steps)
{
	return ((value << steps) | (value >> (32 - steps)));
}

REFLEX_INLINE void SHA1_ClearBuffer(UInt * buffert)
{
	for (Int pos = 16; --pos >= 0;) buffert[pos] = 0;
}

void SHA1_InnerHash(UInt * result, UInt * w)
{
	UInt a = result[0];
	UInt b = result[1];
	UInt c = result[2];
	UInt d = result[3];
	UInt e = result[4];

	Int round = 0;

	while (round < 16)
	{
		SHA1_MACRO((b & c) | (~b & d), 0x5a827999)
	}

	while (round < 20)
	{
		w[round] = SHA1_Rol((w[round - 3] ^ w[round - 8] ^ w[round - 14] ^ w[round - 16]), 1);

		SHA1_MACRO((b & c) | (~b & d), 0x5a827999)
	}

	while (round < 40)
	{
		w[round] = SHA1_Rol((w[round - 3] ^ w[round - 8] ^ w[round - 14] ^ w[round - 16]), 1);

		SHA1_MACRO(b ^ c ^ d, 0x6ed9eba1)
	}

	while (round < 60)
	{
		w[round] = SHA1_Rol((w[round - 3] ^ w[round - 8] ^ w[round - 14] ^ w[round - 16]), 1);

		SHA1_MACRO((b & c) | (b & d) | (c & d), 0x8f1bbcdc)
	}

	while (round < 80)
	{
		w[round] = SHA1_Rol((w[round - 3] ^ w[round - 8] ^ w[round - 14] ^ w[round - 16]), 1);

		SHA1_MACRO(b ^ c ^ d, 0xca62c1d6)
	}

	#undef SHA1_MACRO

	result[0] += a;
	result[1] += b;
	result[2] += c;
	result[3] += d;
	result[4] += e;
}

void SHA1_Calculate(const UInt8 * input, const Int bytelength, UInt8 * output)
{
	UInt result[5] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0 };

	UInt w[80];

	const Int end = bytelength - 64;

	Int current_block_end;

	Int current_block = 0;

	while (current_block <= end)
	{
		current_block_end = current_block + 64;

		for (Int roundPos = 0; current_block < current_block_end; current_block += 4)
		{
			w[roundPos++] = (UInt) input[current_block + 3] | (((UInt) input[current_block + 2]) << 8) | (((UInt) input[current_block + 1]) << 16) | (((UInt) input[current_block]) << 24);
		}

		SHA1_InnerHash(result, w);
	}

	current_block_end = bytelength - current_block;

	SHA1_ClearBuffer(w);

	Int last_block_bytes = 0;

	for (;last_block_bytes < current_block_end; ++last_block_bytes)
	{
		w[last_block_bytes >> 2] |= (UInt) input[last_block_bytes + current_block] << ((3 - (last_block_bytes & 3)) << 3);
	}

	w[last_block_bytes >> 2] |= 0x80 << ((3 - (last_block_bytes & 3)) << 3);

	if (current_block_end >= 56)
	{
		SHA1_InnerHash(result, w);

		SHA1_ClearBuffer(w);
	}

	w[15] = bytelength << 3;

	SHA1_InnerHash(result, w);

	for (Int hash_byte = 20; --hash_byte >= 0;)
	{
		output[hash_byte] = (result[hash_byte >> 2] >> (((3 - hash_byte) & 0x3) << 3)) & 0xff;
	}
}

REFLEX_END_INTERNAL

void Reflex::Data::Detail::SHA1(const Archive::View & data, Archive::Region output)
{
	REFLEX_ASSERT(output.size == 20);

	SHA1_Calculate(data.data, data.size, output.data);
}

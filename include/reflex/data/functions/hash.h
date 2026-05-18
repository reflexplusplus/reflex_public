#pragma once

#include "../types.h"




//
//Primary API

namespace Reflex::Data
{

	Archive SHA1(const Archive::View & bytes);

	Archive SHA256(const Archive::View & bytes);


	UInt32 FNV1a32(const Archive::View & bytes, UInt32 seed = 2166136261u);

	UInt64 FNV1a64(const Archive::View & bytes, UInt64 seed = 1469598103934665603ull);


	UInt32 CRC32(const Archive::View & bytes, UInt32 previous = 0);

	UInt32 Checksum32(const Archive::View & bytes, UInt32 previous = 0);

}




//
//impl

REFLEX_NS(Reflex::Data::Detail)

template <auto FN, UInt SIZE> inline Archive Hash(const Archive::View & bytes)
{
	Archive rtn(SIZE);

	FN(bytes, ToRegion(rtn));

	return rtn;
}

void SHA1(const Archive::View & bytes, Archive::Region && output);

void SHA256(const Archive::View & bytes, Archive::Region && output);

REFLEX_END

inline Reflex::Data::Archive Reflex::Data::SHA1(const Archive::View & bytes)
{
	return Detail::Hash<Detail::SHA1,20>(bytes);
}

inline Reflex::Data::Archive Reflex::Data::SHA256(const Archive::View & bytes)
{
	return Detail::Hash<Detail::SHA256,32>(bytes);
}

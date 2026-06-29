#pragma once

#include "string.h"




//
//Primary API

namespace Reflex
{

	constexpr UInt32 MakeKey32(const CString::View & string);


	consteval UInt32 K32(const char * string);

	consteval UInt64 K64(const char * string);

	consteval UInt32 ID32(const char * string);

	consteval UInt64 ID64(const char * string);

	consteval UInt32 CC32(const char * string);


}




//
//impl

REFLEX_NS(Reflex::Detail)

template <class UINT, class CHARACTER> constexpr void IncrementHash(UINT & value, CHARACTER c);

template <class UINT> consteval UINT MakeHashFromNullTerminatedString(const char * string);

template <class UINT, class CHARACTER, UInt LENGTH> consteval UINT MakeHash(const CHARACTER(&string)[LENGTH]);

template <class UINT, class CHARACTER> constexpr UINT MakeHash(const ArrayView <CHARACTER> & string);

template <class UINT, bool IS_LITTLE_ENDIAN> consteval UINT MakeCCFromNullTerminatedString(const char * string);

template <class UINT> constexpr UINT MergeHashes(UINT a, UINT b)
{
	IncrementHash(a, b);

	return a;
}

REFLEX_END

template <class UINT, class CHARACTER> inline constexpr void Reflex::Detail::IncrementHash(UINT & hash, CHARACTER c)
{
	hash = ((hash << 5) + hash) + c;
}

template <class UINT, class CHARACTER, Reflex::UInt N> consteval UINT Reflex::Detail::MakeHash(const CHARACTER(&string)[N])
{
	REFLEX_STATIC_ASSERT(N);

	UINT hash = 5381;

	REFLEX_LOOP(idx, N - 1) IncrementHash(hash, string[idx]);

	return hash;
}

template <class UINT> consteval UINT Reflex::Detail::MakeHashFromNullTerminatedString(const char * string)
{
	UINT hash = 5381;

	while (auto c = *string++) IncrementHash(hash, c);

	return hash;
}

template <class UINT, class CHARACTER> inline constexpr UINT Reflex::Detail::MakeHash(const ArrayView <CHARACTER> & ref)
{
	UINT hash = 5381;

	auto ptr = ref.data;

	auto end = ptr + ref.size;

	while (ptr < end) IncrementHash(hash, *ptr++);

	return hash;
}

template <class UINT, bool IS_LITTLE_ENDIAN> consteval UINT Reflex::Detail::MakeCCFromNullTerminatedString(const char * string)
{
	UINT cc = 0;

	UINT shift = 1;

	if constexpr (IS_LITTLE_ENDIAN)
	{
		shift = (sizeof(UINT) == 8) ? (0xFFFFFFFFFFFFFFull + 1) : 0xFFFFFF + 1;
	}

	while (UINT c = *string++)
	{
		c *= shift;

		cc |= c;

		if constexpr (IS_LITTLE_ENDIAN)
		{
			shift /= 256;
		}
		else
		{
			shift *= 256;
		}
	}

	return cc;
}

inline constexpr Reflex::UInt32 Reflex::MakeKey32(const CString::View & string)
{
	return Detail::MakeHash<UInt32>(string);
}

consteval Reflex::UInt32 Reflex::K32(const char * string)
{
	return Detail::MakeHashFromNullTerminatedString<UInt32>(string);
}

consteval Reflex::UInt64 Reflex::K64(const char * string)
{
	return Detail::MakeHashFromNullTerminatedString<UInt64>(string);
}

consteval Reflex::UInt32 Reflex::ID32(const char * string)
{
	return Detail::MakeCCFromNullTerminatedString<UInt32, false>(string);
}

consteval Reflex::UInt64 Reflex::ID64(const char * string)
{
	return Detail::MakeCCFromNullTerminatedString<UInt64, false>(string);
}

consteval Reflex::UInt32 Reflex::CC32(const char * string)
{
	return Detail::MakeCCFromNullTerminatedString<UInt32, true>(string);
}




//
//deferred key definitions

template <class UINT> template <Reflex::UInt N> consteval Reflex::Key<UINT>::Key(const char(&string)[N])
	: value(Detail::MakeHash<UINT,char,N>(string))
{
}

template <class UINT> consteval Reflex::Key<UINT>::Key(const char * string)
	: value(Detail::MakeHashFromNullTerminatedString<UINT>(string))
{
}

template <class UINT> REFLEX_INLINE Reflex::Key<UINT>::Key(const CString & string)
	: value(Detail::MakeHash<UINT>(ToView(string)))
{
}

template <class UINT> REFLEX_INLINE Reflex::Key<UINT>::Key(const CString::View & stringref)
	: value(Detail::MakeHash<UINT>(stringref))
{
}

template <class UINT> REFLEX_INLINE Reflex::Key<UINT>::Key(const WString & string)
	: value(Detail::MakeHash<UINT>(ToView(string)))
{
}

template <class UINT> REFLEX_INLINE Reflex::Key<UINT>::Key(const WString::View & stringref)
	: value(Detail::MakeHash<UINT>(stringref))
{
}

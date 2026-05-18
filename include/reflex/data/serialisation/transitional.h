#pragma once

#include "pack.h"
#include "store.h"




//
//declarations

namespace Reflex::Data
{

	template <class TYPE> Data::Archive ToBinary(const TYPE & value);

	template <class TYPE> void FromBinary(Data::Archive::View view, TYPE & value);

	template <class TYPE> TYPE FromBinary(Data::Archive::View view);


	template <class TYPE> const TYPE & Peek(Archive::View & stream);

	template <class TYPE> ArrayView <TYPE> ReadRawArray(Archive::View & stream, UInt n);

	inline ArrayView <UInt8> ReadBytes(Archive::View & stream, UInt n) { return ReadRawArray<UInt8>(stream, n); }

}

REFLEX_NS(Reflex::Data::Detail)

void RestoreLegacyWString(Archive::View & stream, WString & out);

REFLEX_END




//
//impl

template <class TYPE> inline Reflex::Data::Archive Reflex::Data::ToBinary(const TYPE & value)
{
	static_assert(!IsType<TYPE, WString, WString::View>::value, "use Data::EncodeUCS2");

	if constexpr (Detail::kIsRawPackable<TYPE>)
	{
		return Pack(value);
	}
	else
	{
		Archive rtn;

		Serialize(rtn, value);

		return rtn;
	}
}

template <class TYPE> inline void Reflex::Data::FromBinary(Data::Archive::View view, TYPE & value)
{
	static_assert(!IsType<TYPE, WString, WString::View>::value, "use Data::EncodeUCS2");

	if constexpr (Detail::kIsRawPackable<TYPE>)
	{
		Unpack(view, value);
	}
	else
	{
		Deserialize(view, value);
	}
}

template <class TYPE> inline TYPE Reflex::Data::FromBinary(Data::Archive::View view)
{
	static_assert(!IsType<TYPE, WString, WString::View>::value, "use Data::EncodeUCS2");

	if constexpr (Detail::kIsRawPackable<TYPE>)
	{
		return Unpack<TYPE>(view);
	}
	else
	{
		return Deserialize<TYPE>(view);
	}
}

template <class TYPE> REFLEX_INLINE const TYPE & Reflex::Data::Peek(Archive::View & stream)
{
	REFLEX_STATIC_ASSERT(IsRawCopyable<TYPE>::value);

	REFLEX_ASSERT(stream.size >= sizeof(TYPE));

	return *Reinterpret<TYPE>(stream.data);
}

template <class TYPE> REFLEX_INLINE Reflex::ArrayView <TYPE> Reflex::Data::ReadRawArray(Archive::View & stream, UInt n)
{
	REFLEX_STATIC_ASSERT(IsRawCopyable<TYPE>::value);

	auto raw = Inc(stream, sizeof(TYPE) * n);

	raw.size = n;

	return Reinterpret< ArrayView<TYPE> >(raw);
}

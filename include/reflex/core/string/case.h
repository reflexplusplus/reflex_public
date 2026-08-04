#pragma once

#include "string.h"




//
//Primary API

namespace Reflex
{

	template <class STRING> auto Lowercase(const STRING & string);

	template <class STRING> auto Uppercase(const STRING & string);


	char Lowercase(char character);

	char16_t Lowercase(char16_t character);

	WChar Lowercase(WChar character);


	char Uppercase(char character);

	char16_t Uppercase(char16_t character);

	WChar Uppercase(WChar character);

}




//
//impl

REFLEX_NS(Reflex::Detail)

template <class CHARACTER> REFLEX_INLINE CHARACTER RemapCharacter(const UInt8 map[128], CHARACTER character)
{
	if (character < 256)
	{
		return map[character];
	}
	else
	{
		return character;
	}
}

CString RemapString(const UInt8 map[256], const CString::View & string);

CString16 RemapString(const UInt8 map[256], const CString16::View & string);

WString RemapString(const UInt8 map[256], const WString::View & string);

extern const UInt8 kLowerCase[256];

extern const UInt8 kUpperCase[256];

REFLEX_END

REFLEX_INLINE char Reflex::Lowercase(char character)
{
	return Detail::kLowerCase[character];
}

REFLEX_INLINE char Reflex::Uppercase(char character)
{
	return Detail::kUpperCase[character];
}

REFLEX_INLINE char16_t Reflex::Lowercase(char16_t character)
{
	return Detail::RemapCharacter(Detail::kLowerCase, character);
}

REFLEX_INLINE char16_t Reflex::Uppercase(char16_t character)
{
	return Detail::RemapCharacter(Detail::kUpperCase, character);
}

REFLEX_INLINE Reflex::WChar Reflex::Lowercase(WChar character)
{
	return Detail::RemapCharacter(Detail::kLowerCase, character);
}

REFLEX_INLINE Reflex::WChar Reflex::Uppercase(WChar character)
{
	return Detail::RemapCharacter(Detail::kUpperCase, character);
}

template <class STRING> REFLEX_INLINE auto Reflex::Lowercase(const STRING & string)
{
	return Detail::RemapString(Detail::kLowerCase, ToView(string));
}

template <class STRING> REFLEX_INLINE auto Reflex::Uppercase(const STRING & string)
{
	return Detail::RemapString(Detail::kUpperCase, ToView(string));
}

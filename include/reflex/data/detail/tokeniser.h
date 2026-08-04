#pragma once

#include "../[require].h"




//
//declarations

REFLEX_NS(Reflex::Data::Detail)

enum CharType : UInt8
{
	kCharTypeNull,

	kCharTypeSymbol,

	kCharTypeBracket,	// {}, (), [] (not <>)

	kCharTypeWord,
	kCharTypeNumber,
	kCharTypeHex,

	kCharTypeSingleQuotedString,
	kCharTypeDoubleQuotedString,

	kCharTypeLF,
	kCharTypeCR,
	kCharTypeWhiteSpace,
	kCharTypeBracketClose,
};

struct Tokeniser;


void TokeniseFail(UInt line, const CString::View & text);


Idx SearchCharacter(const CString::View & text, char character);

Idx SearchCharacter(const CString::View & string, char character);


bool IsAlphaCharacter(char c);

bool IsAlphaNumericCharacter(char c);


template <auto FN, class ... VARGS> CString::View IterateWhile(CString::View & buffer, VARGS &&... v);

template <auto FN, class ... VARGS> CString::View ExtractWhile(CString::View & buffer, VARGS &&... v);


CString::View ReadLine(CString::View & itr);

WString::View ReadLine(WString::View & itr);


extern CharType kChar2Type[128];

REFLEX_END





//
//Detail::Tokeniser

struct Reflex::Data::Detail::Tokeniser : public Object
{
	static Tokeniser & null;

	enum ValueType : UInt8
	{
		kValueTypeWord = 4,
		kValueTypeInt,
		kValueTypeFloat,
		kValueTypeHex,

		kValueTypeSingleQuotedString,
		kValueTypeDoubleQuotedString,

		kNumValueType,
	};

	static void Tokenise(Tokeniser & tokeniser, const CString::View & input, UInt & line);	 //throw Pair <UInt,CString>

	virtual TRef <Tokeniser> OnPush(UInt line, const char & bracket) = 0;

	virtual void OnPop(UInt line, const char & bracket, Tokeniser & child) = 0;

	virtual void OnComment(UInt line, const CString::View & comment) = 0;

	virtual void OnValue(UInt line, ValueType type, const CString::View & word) = 0;

	virtual void OnSymbol(UInt line, const char & symbol) = 0;
};

REFLEX_SET_TRAIT(Data::Detail::Tokeniser, IsSingleThreadExclusive);




//
//impl

REFLEX_INLINE bool Reflex::Data::Detail::IsAlphaCharacter(char c)
{
	//return kChar2Type[c] == kCharTypeWord;

	UInt32 lo = (UInt(c) | 0x20) - UInt32('a');

	return (lo <= 25);
}

REFLEX_INLINE bool Reflex::Data::Detail::IsAlphaNumericCharacter(char c)
{
	auto type = kChar2Type[c];

	return type == kCharTypeWord || type == kCharTypeNumber;
}

template <auto FN, class ... VARGS> REFLEX_INLINE Reflex::CString::View Reflex::Data::Detail::IterateWhile(CString::View & buffer, VARGS &&... v)
{
	while (buffer.size && FN(*buffer.data, std::forward<VARGS>(v)...))
	{
		buffer.data++;

		buffer.size--;
	}

	return buffer;
}

template <auto FN, class ... VARGS> REFLEX_INLINE Reflex::CString::View Reflex::Data::Detail::ExtractWhile(CString::View & buffer, VARGS &&... v)
{
	auto from = buffer;

	IterateWhile<FN>(buffer, std::forward<VARGS>(v)...);

	from.size = (from.size - buffer.size);

	return from;
}

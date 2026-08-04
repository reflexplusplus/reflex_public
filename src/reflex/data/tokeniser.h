#pragma once

#include "functions.h"

#define DATA_DECLARE_ERROR(a, b) inline const CString::View kError##a = b




//
//

REFLEX_NS(Reflex::Data::Detail)

DATA_DECLARE_ERROR(UnexpectedSymbol, "unexpected symbol");
DATA_DECLARE_ERROR(ParseError, "parse error");
DATA_DECLARE_ERROR(ScopeMismatch, "scope mismatch");


CString::View IterateString(CString::View & buffer, bool singlequote);

CString::View & PreInc(CString::View & buffer);


bool IsNotQuote(char c);

bool IsNotSingleQuote(char c);

REFLEX_END




//
//impl

REFLEX_INLINE bool Reflex::Data::Detail::IsNotQuote(char c)
{
	return c != '"';
}

REFLEX_INLINE bool Reflex::Data::Detail::IsNotSingleQuote(char c)
{
	return c != 39;
}

REFLEX_INLINE Reflex::CString::View & Reflex::Data::Detail::PreInc(CString::View & buffer)
{
	buffer.data++;

	buffer.size--;

	return buffer;
}

REFLEX_INLINE Reflex::CString::View Reflex::Data::Detail::IterateString(CString::View & buffer, bool singlequote)
{
	auto from = buffer;

	if (singlequote ? IterateWhile<IsNotSingleQuote>(buffer) : IterateWhile<IsNotQuote>(buffer))
	{
		if (buffer.size >= 1)
		{
			from.size = (from.size - buffer.size);

			buffer = Nudge(buffer);

			return from;
		}
	}

	return {};
}

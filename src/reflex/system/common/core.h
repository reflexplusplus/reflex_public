#pragma once

#include "[require].h"




//
//offically part of Reflex::Data & Reflex::File, but needed for internal implementation
//implement here to avoid internal dependency of Reflex::System on Reflex::Data & Reflex::File

REFLEX_NS(Reflex::Data)

void EncodeUTF8(Array <UInt8> & output, const WString::View & utf8);

void DecodeUTF8(WString & output, const ArrayView <UInt8> & utf);

REFLEX_END

REFLEX_NS(Reflex::Data::Detail)

CString::View ReadLine(CString::View & stream);

WString::View ReadLine(WString::View & stream);

REFLEX_END

REFLEX_NS(Reflex::File)

Pair <WString::View> SplitExtension(const WString::View & path);

REFLEX_END

REFLEX_NS(Reflex::File::Detail)

void CorrectStrokes(WString & path);

void CorrectTrailingStroke(WString & path);

void RemoveTrailingStroke(WString & path);

REFLEX_END

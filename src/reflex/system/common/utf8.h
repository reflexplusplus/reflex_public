#pragma once

#include "core.h"





REFLEX_NS(Reflex::System::Common)

typedef Array <char> UTF8;

UTF8 ToUTF8(const WString::View & s);

WString DecodeUTF8(const ArrayView <UInt8> & utf8);

REFLEX_END




//
//impl

inline Reflex::WString Reflex::System::Common::DecodeUTF8(const ArrayView <UInt8> & utf8)
{
	WString rtn;

	Data::DecodeUTF8(rtn, utf8);

	return rtn;
}

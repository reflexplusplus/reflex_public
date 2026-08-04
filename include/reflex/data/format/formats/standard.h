#pragma once

#include "../format.h"




//
//Primary API

namespace Reflex::Data
{

	extern const ConstTRef <Format> kRiffFormat;	//TODO -> SerializableFormat


	enum JsonFormatOptions : UInt8
	{
		kJsonFormatOptionInt64 = MakeBit(0),
		kJsonFormatOptionFloat64 = MakeBit(1),
		kJsonFormatOptionWString = MakeBit(2)	//use for UTF8 json
	};

	extern const ConstTRef <Format> kJsonFormat;


	extern const ConstTRef <Format> kHttpQueryFormat;

}

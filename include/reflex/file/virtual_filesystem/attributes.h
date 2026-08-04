#pragma once

#include "../[require].h"




//
//Secondary API

namespace Reflex::File
{

	struct Attributes;

}




//
//Attributes

struct Reflex::File::Attributes
{
	enum Status : UInt8
	{
		kStatusMissing,
		kStatusStreaming,
		kStatusReady,
	};

	Status status = kStatusMissing;

	Pair <UInt64> size_time;

	WString resolved_path;
};

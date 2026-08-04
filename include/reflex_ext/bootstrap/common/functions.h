#pragma once

#include "[require].h"




//
//Primary API

namespace Reflex::Bootstrap
{

	inline bool IsPlugin() { return System::kEnvironmentType == System::kEnvironmentTypeAudioPlugin; }

	template <class TYPE> inline TYPE UnpackResource(Key32 group, Key32 id, const TYPE & fallback);	//reads a value from EnumerableEmbeddedResource "Config" group

}




//
//impl

template <class TYPE> inline TYPE Reflex::Bootstrap::UnpackResource(Key32 group, Key32 id, const TYPE & fallback)
{
	if (auto res = File::EnumerableEmbeddedResource::Retrieve({ group, id }))
	{
		return Data::FromBinary<TYPE>(res->data);
	}

	return fallback;
};

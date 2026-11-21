#pragma once

#include "../defines.h"




//
//Experimental API

namespace Reflex::System
{

	class SecretStorage;	//os backed secure storage for credentials, degree of "secureness" is platform specific

}




//
//SecretStorage

class Reflex::System::SecretStorage : public Object
{
public:
		
	REFLEX_OBJECT(SecretStorage, Object);

	static SecretStorage & null;

	enum Result : UInt8
	{
		kResultSuccess,
		kResultNotFound,
		kResultDenied,
		kResultTooLarge,
		kResultFailed
	};

	static constexpr UInt kMaxKeySize = 255;	//applies to storage_name + keys
	static constexpr UInt kMaxValueSize = 64 * 1024;

	//Windows requires an existing absolute local directory that the caller keeps private and dedicated to this store. Include a trailing '/'.
	[[nodiscard]] static Unretained <SecretStorage> Create(CString::View storage_name, FunctionPointer <WString(CString::View storage_name)> get_path);

	//Calls are synchronous and some platforms may prompt
	[[nodiscard]] virtual Result Unset(CString::View key) const = 0;	//Returns NotFound when absent.
	[[nodiscard]] virtual Result Set(CString::View key, ArrayView <UInt8> value) const = 0;
	[[nodiscard]] virtual Result Get(CString::View key, Array <UInt8> & value) const = 0;	//Clears value on failure.
};

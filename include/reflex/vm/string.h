#pragma once

#include "traits.h"




//
//Experimental API

namespace Reflex::VM
{

	class String;

};




//
//String

class Reflex::VM::String : public Object
{
public:

	REFLEX_OBJECT(VM::String,Object);

	static String & null;


	[[nodiscard]] static TRef <String> Create(const WString::View & value, Key32 hash);

	[[nodiscard]] static TRef <String> Create(const WString::View & value) { return Create(value, Key32(value)); }

	[[nodiscard]] static TRef <String> Create(const CString::View & value, Key32 hash);

	[[nodiscard]] static TRef <String> Create(const CString::View & value) { return Create(value, Key32(value)); }

	[[nodiscard]] static TRef <String> Create(UInt size);	//special, need to manually set hash again, and null terminate


	bool operator==(const String & string) const { return hash == string.hash && GetView() == string.GetView(); }


	WString::View GetView() const { return { data + 0, size }; }

	const Key32 hash;

	const UInt32 size;

	WChar data[1];



protected:

	friend Reflex::Detail::Constructor <String>;

	String(Key32 hash, UInt size)
		: hash(hash),
		size(size)
	{
	}
};

REFLEX_SET_TRAIT(VM::String, IsAbstract);
REFLEX_SET_TRAIT(VM::String, IsNonCircular);
REFLEX_SET_TRAIT(VM::String, IsThreadSafe);

#pragma once

#include "../types.h"




//
//Secondary API

namespace Reflex::Data
{

	struct iSerializable;

}




//
//iSerializable

struct Reflex::Data::iSerializable
{
public:

	static iSerializable & null;

	//lifetime
	
	iSerializable(UInt16 version) : version(version) {}


	
	//access
	
	void Reset(Key32 context = {});

	void Deserialize(Archive::View & stream, Key32 context = {});

	static void Skip(Archive::View & stream);

	void Serialize(Archive & stream) const;



	//info

	const UInt16 version;



protected:

	virtual void OnReset(Key32 context) {}

	virtual void OnRestore(Archive::View & stream, Key32 context) = 0;

	virtual bool OnImport(UInt16 version, Archive::View & stream, Key32 context) { return false; }

	virtual void OnStore(Archive & stream) const = 0;

};




//
//impl

REFLEX_NS(Reflex::Data)
typedef iSerializable iStreamable [[deprecated("use iSerializable")]];
REFLEX_END




//
//impl

REFLEX_INLINE void Reflex::Data::iSerializable::Reset(Key32 context)
{
	OnReset(context);
}

#pragma once

#include "[require].h"




//
//Primary API

namespace Reflex::File
{

	class Monolith;

}




//
//Monolith

class Reflex::File::Monolith : public Object
{
public:

	REFLEX_OBJECT(Reflex::File::Monolith,Object);

	static Monolith & null;



	//header

	static const UInt32 kHeader;



	//lifetime

	[[nodiscard]] static TRef <Monolith> Create(const WString::View & filename, UInt32 clientheader, bool write);

	[[nodiscard]] static TRef <Monolith> Create(System::FileHandle & file, UInt32 clientheader);



	//info

	virtual bool IsWriteable() const = 0;

	virtual bool Status() const = 0;



	//write

	virtual void Clear() = 0;

	virtual bool Remove(Key64 partitionid) = 0;

	virtual TRef <System::FileHandle> Write(Key64 partitionid, UInt32 size) = 0;

	virtual void Commit() = 0;



	//read

	virtual void Enumerate(const Function <void(Key64,UInt32)> & callback) const = 0;

	virtual TRef <System::FileHandle> Read(Key64 partitionid) const = 0;

};

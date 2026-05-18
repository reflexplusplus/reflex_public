#pragma once

#include "types.h"




//
//Secondary API

namespace Reflex::System
{

	class FileHandle;

}




//
//FileHandle

class Reflex::System::FileHandle : public Object
{
public:

	REFLEX_OBJECT(System::FileHandle, Object);

	static FileHandle & null;



	//mode

	enum Mode : UInt8
	{
		kModeRead,
		kModeOverwrite,
		kModeAppend,
	};

	enum StandardStream : UInt8
	{
		kStandardStreamIn,
		kStandardStreamOut,
	};



	//lifetime

	[[nodiscard]] static TRef <FileHandle> Create(const WString & path, Mode mode = kModeRead, bool lock = true);

	[[nodiscard]] static TRef <FileHandle> Create(StandardStream stream);



	//interface

	virtual bool Status() const = 0;

	virtual bool IsWriteable() const = 0;


	virtual UInt64 GetSize() const = 0;


	virtual void SetPosition(UInt64 position) = 0;

	virtual UInt64 GetPosition() const = 0;


	virtual UInt32 Read(void * ptr, UInt32 max_size) = 0;

	virtual UInt32 Write(const void * ptr, UInt size) = 0;


	virtual bool Truncate() = 0;

	virtual void Flush() = 0;

};

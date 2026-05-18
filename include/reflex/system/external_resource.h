#pragma once

#include "file_handle.h"




//
//Experimental API

namespace Reflex::System
{

	class ExternalResourceRef;

}




//
//ExternalResourceRef

class Reflex::System::ExternalResourceRef : public Object
{
public:

	static ExternalResourceRef & null;

	enum AccessMode : UInt8 
	{
		kAccessModeReadOnly = 1,
		kAccessModeCreateNew = 2,
		kAccessModeReadWrite = 3
	};

	[[nodiscard]] static TRef <ExternalResourceRef> Locate(const ArrayView <UInt8> & token);

	virtual Array <UInt8> GetPersistentToken() = 0;

	virtual TRef <FileHandle> Open(FileHandle::Mode mode) = 0;
};

#pragma once

#include "types.h"




//
//Secondary API

namespace Reflex::System
{

	class FileHandle;

	
	UIntNative GetThreadID();

	void DebugLog(bool brk, const char * msg);

	Float64 GetElapsedTime();

}

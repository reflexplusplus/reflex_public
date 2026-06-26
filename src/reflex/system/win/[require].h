#pragma once

#include "../common/module.h"
#include "../common/debug.h"
#include "../common/core.h"




//
//windows sdk

#define storenew new

#undef new

#define WINDOWS_LEAN_AND_MEAN
struct IUnknown;	//workaround a windows bug Error C2760 in combaseapi.h with Windows SDK 8.1 and compiler flag /permissive-

REFLEX_DISABLE_WARNINGS
#include <windows.h>
REFLEX_ENABLE_WARNINGS

#include <fcntl.h>
#include <io.h>
//#include <ctime>	//for time on vc c++17
#include <iostream>

#include <shlobj.h>
#include <winhttp.h>

#undef CreateFile
#undef CreateDirectory
#undef GetCommandLine
#undef GetFileAttributes
#undef GetFileAttributesEx
#undef SetCurrentDirectory
#undef GetCurrentDirectory
#undef GetObject
#undef SendMessage




//
//define

#define STDCALL __stdcall




//
//helper funtions

REFLEX_NS(Reflex::System::Win)

template <class TYPE> inline TYPE & Init(TYPE & type)
{
	MemClear(&type, sizeof(TYPE));

	return type;
}

template <class TYPE> inline TYPE Init()
{
	TYPE type;

	return Init(type);
}

REFLEX_END

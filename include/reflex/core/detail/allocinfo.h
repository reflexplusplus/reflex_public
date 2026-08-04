#pragma once

#include "../meta/auxtypes.h"
#include "../array/view.h"




//
//macros

#if (REFLEX_DEBUG)

#define REFLEX_ALLOCINFO(TYPE) Reflex::AllocInfo(TYPE::kDynamicTypeInfo.tname, __FILE__, __LINE__ )

#else

#define REFLEX_ALLOCINFO(TYPE) Reflex::AllocInfo()

#endif




//
//declaration

namespace Reflex
{

	struct AllocInfo;

}

//REFLEX_NS(Reflex::Detail)
//
//template <class TYPE> class Constructor;
//
//REFLEX_END




//
//AllocInfo

#if (REFLEX_DEBUG)

struct Reflex::AllocInfo
{
	AllocInfo(const char * type = "unknown", const char * file = "", UInt line = 0)
		: type(type)
		, file(file)
		, line(line)
	{
	}

	const char * type;

	const char * file;

	UInt line;
};

#else

struct Reflex::AllocInfo 
{
	AllocInfo(const char * type = 0, const char * file = 0, UInt line = 0)
	{
	}
};

#endif

#pragma once




//
//dependencies

#include "common.h"
#include "intel.h"

#include <intrin.h>	//for intrin memset




//
//env

#if defined(_CPPRTTI)
#define REFLEX_RTTI_ENABLED
#endif

#ifdef _WIN64
#define REFLEX_64BIT 1
#else
#define REFLEX_64BIT 0
#endif

#ifdef _DEBUG
#define REFLEX_DEBUG 1
#else
#define REFLEX_DEBUG 0
#endif




//
//macros

#define REFLEX_DISABLE_WARNINGS __pragma(warning(push, 0))

#define REFLEX_ENABLE_WARNINGS __pragma(warning(pop))


#define REFLEX_FORCEINLINE __forceinline

#define REFLEX_NOINLINE __declspec(noinline)


#define REFLEX_ALLOC16(size) _aligned_malloc(size, 16)

#define REFLEX_FREE16(ptr) _aligned_free(ptr)

#define REFLEX_STACKALLOC(size) _alloca(size)


#define REFLEX_EXPORT __declspec(dllexport)

#define REFLEX_STDCALL __stdcall




//
//warnings

#pragma intrinsic(memset, memcpy, memcmp)

#pragma warning (disable : 4100)	//unused function arg
#pragma warning (disable : 4706)	//assignment within conditional, cannot be solved by brackets on VC
#pragma warning (disable : 4250)	//inheriance via dominance -> invalid warnings on virtual inheritance
#pragma warning (disable : 4324)	//structure padded due to alignment -> absurd warning

//move to internal (or quicker hack fix -> during REFLEX_BUILD)

#pragma warning (disable : 4456)	//reuse variable name
#pragma warning (disable : 4457)	//reuse variable name
#pragma warning (disable : 4458)	//reuse variable name
#pragma warning (disable : 4459)	//reuse variable name

#pragma warning (disable : 4701)	//potentially uninitialized -> often invalid
#pragma warning (disable : 4702)	//unreachable code -> often invalid

#pragma warning(3 : 4189)	// local variable initialized but not referenced
#pragma warning(3 : 4505)	// unreferenced local function removed
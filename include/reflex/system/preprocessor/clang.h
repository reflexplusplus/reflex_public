#pragma once




//
//dependencies

#include "common.h"




//
//env

#if __has_feature(cxx_rtti)
	#define REFLEX_RTTI_ENABLED
#endif

#ifdef __LP64__
	#define REFLEX_64BIT 1
#else
	#define REFLEX_64BIT 0
#endif

#ifdef __OPTIMIZE__
	#define REFLEX_DEBUG 0
#else
	#define REFLEX_DEBUG 1
#endif




//
//keywords

#define REFLEX_DISABLE_WARNINGS _Pragma("clang diagnostic push"); _Pragma("clang diagnostic ignored \"-Weverything\"");

#define REFLEX_ENABLE_WARNINGS _Pragma("clang diagnostic pop")


#define REFLEX_FORCEINLINE __attribute((always_inline)) inline

#define REFLEX_NOINLINE __attribute__ ((noinline))


#define REFLEX_ALLOC16(size) malloc(size)

#define REFLEX_FREE16(ptr) free(ptr)

#define REFLEX_STACKALLOC(size) alloca(size)


#define REFLEX_EXPORT __attribute__ ((visibility ("default")))

#define REFLEX_STDCALL




//
//warnings

#undef ALIGN

#pragma clang diagnostic ignored "-Wconstant-logical-operand"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wswitch"
#pragma clang diagnostic ignored "-Winvalid-offsetof"

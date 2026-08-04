#pragma once




//
//dependencies

#include "common.h"




//
//env

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
//macros

#define REFLEX_DISABLE_WARNINGS \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wunused-variable\"") \
  _Pragma("GCC diagnostic ignored \"-Wmisleading-indentation\"") \
  _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"") \
  _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"") \
  _Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"")

#define REFLEX_ENABLE_WARNINGS _Pragma("GCC diagnostic pop")


#define REFLEX_FORCEINLINE __attribute((always_inline)) inline

#define REFLEX_NOINLINE __attribute__((noinline))


#define REFLEX_ALLOC16(size) malloc(size)

#define REFLEX_FREE16(ptr) free(ptr)

#define REFLEX_STACKALLOC(size) alloca(size)


#define REFLEX_EXPORT __attribute__ ((visibility ("default")))

#define REFLEX_STDCALL




//
//warnings

#pragma GCC diagnostic ignored "-Wchanges-meaning"
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#pragma GCC diagnostic ignored "-Wchar-subscripts"
#pragma GCC diagnostic ignored "-Wswitch"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wcast-function-type"
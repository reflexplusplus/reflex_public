#pragma once




//
//dependencies

#define REFLEX_ARCH_WEBASM

#include "common.h"
#include <cstdlib>
#include <cstring>



//
//env

#if defined(_CPPRTTI)
#define REFLEX_RTTI_ENABLED
#endif

#define REFLEX_64BIT 0
static_assert(sizeof(void*) == 4, "Pointer size is not 4 bytes");

#ifdef _DEBUG
#define REFLEX_DEBUG 1
#else
#define REFLEX_DEBUG 0
#endif




//
//keywords

#define REFLEX_DISABLE_WARNINGS

#define REFLEX_ENABLE_WARNINGS


#define REFLEX_FORCEINLINE __attribute__((always_inline)) inline

#define REFLEX_NOINLINE __attribute__((noinline))


#define REFLEX_ALLOC16(size) aligned_malloc(size)

#define REFLEX_FREE16(ptr_nonnull) aligned_free(ptr_nonnull)

#define REFLEX_STACKALLOC(size) alloca(size)


#define REFLEX_EXPORT __attribute__((visibility("default")))


#pragma clang diagnostic ignored "-Wconstant-logical-operand"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wswitch"
#pragma clang diagnostic ignored "-Wnonportable-include-path"
#pragma clang diagnostic ignored "-Winvalid-offsetof"




inline void * aligned_malloc(size_t size)
{
	size_t total_size = size + sizeof(void*) + 16;

	void * ptr = malloc(total_size);

	if (ptr == nullptr) return nullptr;

	uintptr_t aligned_addr = (reinterpret_cast<uintptr_t>(ptr) + sizeof(void*) + 15) & ~static_cast<uintptr_t>(15);

	void ** header = reinterpret_cast<void**>(aligned_addr) - 1;

	*header = ptr;

	return reinterpret_cast<void*>(aligned_addr);
}

inline void aligned_free(void * ptr_nonnull)
{
	void * original_ptr = reinterpret_cast<void**>(ptr_nonnull)[-1];

	free(original_ptr);
}

#pragma once

#ifndef REFLEX_USE_STD_FUNCTION
#define REFLEX_USE_STD_FUNCTION 0
#endif

#ifndef REFLEX_USE_BUILTIN_MATH
#define REFLEX_USE_BUILTIN_MATH 1
#endif




//
//dependencies

#define REFLEX_ARCH_WEBASM

#include "clang.h"
#include <cstdlib>
#include <cstring>



//
//env

static_assert(sizeof(void*) == 4, "Pointer size is not 4 bytes");

// Preserve explicit Web build configuration rather than infer it from optimization.
#undef REFLEX_DEBUG
#ifdef _DEBUG
#define REFLEX_DEBUG 1
#else
#define REFLEX_DEBUG 0
#endif




//
//keywords

// Reflex allocations require 16-byte alignment, independently of libc malloc.

#undef REFLEX_ALLOC16
#undef REFLEX_FREE16

#define REFLEX_ALLOC16(size) aligned_malloc(size)
#define REFLEX_FREE16(ptr) aligned_free(ptr)

#pragma clang diagnostic ignored "-Wnonportable-include-path"




inline void * aligned_malloc(size_t size)
{
	constexpr size_t overhead = sizeof(void*) + 15;
	if (size > SIZE_MAX - overhead) return nullptr;
	size_t total_size = size + overhead;

	void * ptr = malloc(total_size);

	if (ptr == nullptr) return nullptr;

	uintptr_t aligned_addr = (reinterpret_cast<uintptr_t>(ptr) + sizeof(void*) + 15) & ~static_cast<uintptr_t>(15);

	void ** header = reinterpret_cast<void**>(aligned_addr) - 1;

	*header = ptr;

	return reinterpret_cast<void*>(aligned_addr);
}

inline void aligned_free(void * ptr_nonnull)
{
	if (ptr_nonnull == nullptr) return;
	void * original_ptr = reinterpret_cast<void**>(ptr_nonnull)[-1];

	free(original_ptr);
}

#pragma once

#include "preprocessor.h"




#ifndef _MM_SHUFFLE
	#define _MM_SHUFFLE(fp3, fp2, fp1, fp0) (((fp3) << 6) | ((fp2) << 4) | ((fp1) << 2) | ((fp0)))
#endif

#if defined (REFLEX_ARM) && defined(__aarch64__)
	#include "simd/arm.h"

#elif defined (REFLEX_ARM) && !defined(__aarch64__)
    #include "simd/polyfill.h"

#elif defined (REFLEX_INTEL)
	#include "simd/intel.h"

#elif defined (REFLEX_ARCH_WEBASM)
	#include "simd/polyfill.h"

#else
	static_assert(false, "unknown platform");

#endif

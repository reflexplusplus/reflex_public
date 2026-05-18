#pragma once




//
//preprocessor

#if defined (__APPLE__)
	#include <TargetConditionals.h>
	#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE //iOS, tvOS, or watchOS
		#define REFLEX_OS_IOS
		#include "preprocessor/ios_arm.h"

	#elif TARGET_OS_MAC
		#define REFLEX_OS_MACOS

		#if defined (__aarch64__)
			#include "preprocessor/macos_arm.h"
		#else
			#include "preprocessor/macos_intel.h"
		#endif
	#endif

#elif defined(__ANDROID__)
	#define REFLEX_OS_ANDROID
	#if __arm__ || __aarch64__
		#include "preprocessor/android_arm.h"
	#elif __i386__ || __x86_64__
		#include "preprocessor/android_intel.h"
	#endif

#elif defined(__wasm__)
	#include "preprocessor/webasm.h"

#elif defined(__GNUC__)
	#define REFLEX_OS_LINUX
	#include "preprocessor/linux_intel.h"

#elif defined(_WIN32) || defined(_WIN64)
	#define REFLEX_OS_WINDOWS
	#include "preprocessor/win_intel.h"

#endif

#if !(defined(REFLEX_INTEL) || defined(REFLEX_ARM) || defined(REFLEX_ARCH_WEBASM))
static_assert(false, "unknown architecture"); 
#endif

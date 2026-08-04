#pragma once

#include "[require].h"




//
//declarations

REFLEX_NS(Reflex::System::Common)

extern Output output;

REFLEX_END




//
//DEV_LOG is for logging for Reflex internal developers

#if (REFLEX_DEBUG) //TODO this is placeholder for REFLEX_BUILD_LIB or something
#define REFLEX_DEV_LOG(OUTPUT, ...) OUTPUT.Log(__VA_ARGS__)
#define REFLEX_DEV_WARN(OUTPUT, ...) OUTPUT.Warn(__VA_ARGS__)
#define REFLEX_DEV_ERROR(OUTPUT, ...) OUTPUT.Error(__VA_ARGS__)
#else
#define REFLEX_DEV_LOG(OUTPUT, ...)
#define REFLEX_DEV_WARN(OUTPUT, ...)
#define REFLEX_DEV_ERROR(OUTPUT, ...)
#endif

#define DEV_LOG(...) REFLEX_DEV_LOG(Reflex::System::Common::output, __VA_ARGS__)
#define DEV_WARN(...) REFLEX_DEV_WARN(Reflex::System::Common::output, __VA_ARGS__)
#define DEV_ERROR(...) REFLEX_DEV_ERROR(Reflex::System::Common::output, __VA_ARGS__); REFLEX_ASSERT(false)

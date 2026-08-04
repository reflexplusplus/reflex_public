#pragma once

#include "preprocessor.h"
#include "system.h"




//
//macros

#if (REFLEX_DEBUG)

#define REFLEX_ASSERT_EX(test, msg) if(!bool(test)) { auto location = std::source_location::current(); Reflex::DebugLog(true, msg, location.file_name(), location.line()); }

#define REFLEX_ASSERT(...) REFLEX_ASSERT_EX(__VA_ARGS__, REFLEX_STRINGIFY(__VA_ARGS__))

#define REFLEX_ASSERT_MAINTHREAD(msg) REFLEX_ASSERT_EX(Reflex::System::GetThreadID() == Reflex::kMainThreadID, msg);

#else

#define REFLEX_ASSERT_EX(test, msg) {}

#define REFLEX_ASSERT(x) {}

#define REFLEX_ASSERT_MAINTHREAD(msg) {}

#endif




//
//impl

REFLEX_NS(Reflex)

#if REFLEX_DEBUG
void DebugLog(bool brk, const char * msg, const char * file, UInt32 line);
#else
inline void DebugLog(bool brk, const char * msg, const char * file, UInt32 line) {}
#endif

extern const UIntNative kMainThreadID;

REFLEX_END

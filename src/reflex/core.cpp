
#include "core/defines.cpp"
#include "core/module.cpp"
#include "core/object.cpp"
#include "core/interface.cpp"
#include "core/weakref.cpp"
#include "core/list.cpp"
#include "core/library.cpp"
#include "core/allocator.cpp"
#include "core/bit.cpp"
#include "core/string.cpp"
#include "core/debug.cpp"

REFLEX_STATIC_ASSERT(sizeof(Reflex::UInt64) == 8);
REFLEX_STATIC_ASSERT(sizeof(Reflex::UIntNative) == sizeof(void*));

REFLEX_NS(Reflex)
bool IsDebug()
{
	return REFLEX_DEBUG;
}
REFLEX_END

#if REFLEX_DEBUG
void Reflex::DebugLog(bool brk, const char * msg, const char * file, UInt32 line)
{
	auto buffer = Detail::DebugJoin({}, msg, ' ', '[', file, ':', line, ']');

	System::DebugLog(brk, buffer.data);
}
#endif

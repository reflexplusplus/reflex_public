#include "debug.h"




REFLEX_STATIC_ASSERT(std::atomic<Reflex::UInt8>::is_always_lock_free);
REFLEX_STATIC_ASSERT(std::atomic<Reflex::UInt32>::is_always_lock_free);
REFLEX_STATIC_ASSERT(std::atomic<Reflex::Float32>::is_always_lock_free);
REFLEX_STATIC_ASSERT(std::atomic<void*>::is_always_lock_free);

Reflex::Output Reflex::System::Common::output("System");

Reflex::FunctionPointer <void(const char * msg)> Reflex::System::g_on_debug_break = [](const char * msg)
{
	printf("*** ASSERT *** %s\n"
		   "To catch on this platform, use...\n"
		   "Reflex::System::g_on_debug_break = [](const char * msg)\n"
		   "{\n"
		   "\t//enable a breakpoint here\n"
		   "};\n",
		   msg);
};

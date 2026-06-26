#include "library.h"




//
//functions

bool Reflex::System::IsDebuggerPresent()
{
	return Win::globals->m_debugger;
}

#if (REFLEX_DEBUG)
void Reflex::System::DebugLog(bool brk, const char * msg)
{
	OutputDebugStringA(msg);

	OutputDebugStringA("\n");

	if (brk) __debugbreak();
}
#endif

void Reflex::System::Log(const char * ptr)
{
	std::cout << ptr;

	std::cout << "\n";
}

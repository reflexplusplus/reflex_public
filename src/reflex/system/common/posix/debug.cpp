#include "[require].h"




#if (REFLEX_DEBUG)
void Reflex::System::DebugLog(bool brk, const char * msg)
{
	printf("%s\n", msg);

	if (brk) g_on_debug_break(msg);
}
#endif

void Reflex::System::Log(const char * text)
{
	printf("%s\n", text);
}

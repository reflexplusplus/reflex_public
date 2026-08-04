#include "[require].h"

Reflex::UInt32 Reflex::System::GetProcessID()
{
	return UInt32(::getpid());
}

Reflex::UInt64 Reflex::System::GetTime()
{
	return time(nullptr);
}

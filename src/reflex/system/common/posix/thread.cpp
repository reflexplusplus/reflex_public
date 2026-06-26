#include "[require].h"

//TODO

Reflex::UIntNative Reflex::System::GetThreadID()
{
	auto tid = pthread_self();

	return tid;
}

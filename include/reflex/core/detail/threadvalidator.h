#pragma once

#include "../functions/cast.h"
#include "../assert.h"




//
//Detail

namespace Reflex::Detail
{

	template <bool ENABLE> struct ThreadValidator {};

}




//
//Detail::ThreadValidator

template <> 
struct Reflex::Detail::ThreadValidator <true>
{
	ThreadValidator()
		: threadid(System::GetThreadID())
	{
	}

	ThreadValidator(const ThreadValidator &)
		: threadid(System::GetThreadID())
	{
	}

	~ThreadValidator()
	{
		REFLEX_ASSERT(System::GetThreadID() == threadid);
	}

	void SetToCurrent()
	{
		RemoveConst(threadid) = System::GetThreadID();
	}

	operator bool() const
	{
		return System::GetThreadID() == threadid;
	}

	const UIntNative threadid;
};

template <> 
struct Reflex::Detail::ThreadValidator <false>
{
	void SetToCurrent() {}

	consteval operator bool() const { return true; }
};

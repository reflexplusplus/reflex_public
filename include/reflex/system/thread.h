#pragma once

#include "defines.h"
#include "task.h"




//
//Secondary API

namespace Reflex::System
{

	class Thread;

	class CriticalSection;


	UIntNative GetThreadID();

	void SuspendThread(UInt ms);

	void YieldThread();

}




//
//Thread

class Reflex::System::Thread : public Task
{
public:

	REFLEX_OBJECT(System::Thread, Task);

	static Thread & null;



	//lifetime

	[[nodiscard]] static TRef <Thread> Create(const Function <void()> & fn, Priority priority = kPriorityNormal, Allocator & allocator = g_default_allocator);

};





//
//CriticalSection

class Reflex::System::CriticalSection : public Object
{
public:

	static CriticalSection & null;

	class Lock;



	//lifetime

	static TRef <CriticalSection> Create(bool recursive = false, Allocator & allocator = g_default_allocator);



	//interface

	virtual void Enter() = 0;

	virtual void Leave() = 0;

};




//
//CriticalSection::Lock

class Reflex::System::CriticalSection::Lock
{
public:

	REFLEX_NONCOPYABLE(Lock);

	Lock(CriticalSection & cs);

	~Lock();


	const TRef <CriticalSection> cs;

};




//
//impl

REFLEX_INLINE Reflex::System::CriticalSection::Lock::Lock(CriticalSection & cs)
	: cs(cs)
{
	cs.Enter();
}

REFLEX_INLINE Reflex::System::CriticalSection::Lock::~Lock()
{
	cs->Leave();
}

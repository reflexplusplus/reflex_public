#include "[require].h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::System::Common)

struct ThreadImpl : public Thread
{
	ThreadImpl(const Function <void()> & fn, Priority priority);

	~ThreadImpl();

	virtual bool Completed() const override { return m_completed; }

	virtual void Wait() override;


	volatile bool m_completed;

	mutable std::thread m_thread;
};

ThreadImpl::ThreadImpl(const Function <void()> & fn, Priority priority)
	: m_completed(false)
	, m_thread([this, fn]()
	{
		fn();
		
		m_completed = true;
		
		//shouldnt need any barrier
		//std::thread guarantees visiblity of prior writes before thread starts and join ensures writes on bg thread visible to joining thread
		//m_completed doesnt need atomics, because worst case its change will just be seen late, which doesnt matter
	})
{
}

ThreadImpl::~ThreadImpl()
{
	if (m_thread.joinable()) m_thread.join();
}

void ThreadImpl::Wait()
{
	if (m_thread.joinable()) m_thread.join();
}

REFLEX_END_INTERNAL

REFLEX_NS(Reflex::System::Common)
extern void SetThreadPriority(Priority priority);
REFLEX_END

Reflex::TRef <Reflex::System::Thread> Reflex::System::Thread::Create(const Function <void()> & fn, Priority priority, Allocator & allocator)
{
	return REFLEX_CREATE_EX(allocator, Common::ThreadImpl, fn, priority);
}

#ifndef  REFLEX_OS_WINDOWS
Reflex::UIntNative Reflex::System::GetThreadID()
{
	REFLEX_STATIC_ASSERT(sizeof(std::thread::id) == sizeof(UIntNative));

	return Reinterpret<UIntNative>(std::this_thread::get_id());
}
#endif

void Reflex::System::SuspendThread(UInt ms)
{
	constexpr unsigned long long mult = 1000000;
	 
	std::chrono::nanoseconds nanoseconds(ms * mult);  // Safe conversion

	std::this_thread::sleep_for(nanoseconds);
}

void Reflex::System::YieldThread()
{
	std::this_thread::yield();
}

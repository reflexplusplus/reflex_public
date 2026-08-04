#include "[require].h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

struct ThreadImpl : public Thread
{
	ThreadImpl(const Function <void()> & fn, Priority priority);

	~ThreadImpl();


	bool Completed() const override { return m_completed; }

	void Wait() override;

	static DWORD WINAPI ThreadProc(LPVOID param);


	volatile bool m_completed = false;

	Function <void()> m_fn;

	HANDLE m_thread;


	static constexpr int kPriorities[] = { THREAD_PRIORITY_LOWEST, THREAD_PRIORITY_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL /*THREAD_PRIORITY_TIME_CRITICAL*/ };

};

ThreadImpl::ThreadImpl(const Function <void()> & fn, Priority priority)
	: m_completed(false)
	, m_fn(fn)
	, m_thread(CreateThread(nullptr, 0, &ThreadProc, this, 0, nullptr))
{
	SetThreadPriority(m_thread, kPriorities[priority]);
}

ThreadImpl::~ThreadImpl()
{
	Wait();
}

void ThreadImpl::Wait()
{
	if (m_thread)
	{
		WaitForSingleObject(m_thread, INFINITE);

		CloseHandle(m_thread);

		m_thread = nullptr;
	}
}

DWORD WINAPI ThreadImpl::ThreadProc(LPVOID param)
{
	auto self = static_cast<ThreadImpl*>(param);

	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	self->m_fn();

	self->m_completed = true;

	if (SUCCEEDED(hr)) CoUninitialize();

	return 0;
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::Thread> Reflex::System::Thread::Create(const Function <void()> & fn, Priority priority, Allocator & allocator)
{
	return REFLEX_CREATE_EX(allocator, Win::ThreadImpl, fn, priority);
}

Reflex::UIntNative Reflex::System::GetThreadID()
{
	UIntNative t = GetCurrentThreadId();

#if REFLEX_DEBUG
	UIntNative std = Reinterpret<UInt32>(std::this_thread::get_id());

	REFLEX_ASSERT(t == std);
#endif

	return t;
}

//void Reflex::System::Common::SetThreadPriority(Priority priority)
//{
//	::SetThreadPriority(::GetCurrentThread(), Win::ThreadImpl::kPriorities[priority]);
//}

void Reflex::System::SuspendThread(UInt ms)
{
	::Sleep(static_cast<DWORD>(ms));
}

void Reflex::System::YieldThread()
{
	if (!::SwitchToThread())
	{
		// Fallback: yield to any thread of equal priority
		::Sleep(0);
	}
}
#include "../../../include/reflex_ext/async/threadpool.h"




//
//implementation

REFLEX_BEGIN_INTERNAL(Reflex::Async)

struct NullThreadPool final : public ThreadPool
{
	Reference <Task> Submit(const TaskFn &) override { return {}; }
	Pair <UInt> GetNumTasks() const override { return {}; }
	void CancelPending() override {}
	void Shutdown() override {}
};

struct ThreadPoolImpl final : public ThreadPool
{
	struct TaskImpl;

	ThreadPoolImpl(UInt max_threads, System::Priority priority);

	~ThreadPoolImpl();


	Reference <Task> Submit(const TaskFn & fn) override;

	Pair <UInt> GetNumTasks() const override;

	void CancelPending() override;

	void Shutdown() override;



private:

	void Worker();

	void UpdateTaskCounts();


	[[maybe_unused]] Reflex::Detail::ThreadValidator <REFLEX_DEBUG> m_threadvalidator;

	const Reference <System::CriticalSection> m_lock;

	List <TaskImpl,true,Task> m_pending, m_active;

	AtomicUInt32 m_task_counts = 0;

	Array <Reference<System::Thread>> m_workers;

	const UInt m_max_threads;
	const System::Priority m_priority;

	UInt m_active_workers = 0;

	bool m_stopping = false;
};

struct ThreadPoolImpl::TaskImpl final : public Item <TaskImpl, true, Task>
{
	struct ContextImpl final : public Worker::Context
	{
		bool Cancelled() const override;

		void SetProgress(Float32 value_normalized) override;

		AtomicFloat32 progress = 0.0f;
		AtomicUInt8 cancelled = false;
	};


	TaskImpl(const ThreadPool::TaskFn & fn);

	~TaskImpl();

	Status GetStatus() const override;

	Float GetProgress() const override;

	AlreadyRetained <Object> GetResult() override;

	void Cancel() override;

	void Wait() override;

	void Execute();

	void CancelBeforeExecution();

	using Item::Attach;
	using Item::Detach;



private:

	void Complete(Worker::Result result);


	ContextImpl m_context;

	ThreadPool::TaskFn m_fn;

	AtomicUInt8 m_status;

	AtomicPointer m_presult;
};

ThreadPoolImpl::ThreadPoolImpl::TaskImpl::TaskImpl(const ThreadPool::TaskFn & fn)
	: m_fn(fn)
	, m_status(kStatusPending)
	, m_presult(&Object::null)
{
	Object::null.RetainMt();
}

ThreadPoolImpl::TaskImpl::~TaskImpl()
{
	Cancel();

	REFLEX_ASSERT(GetStatus() != kStatusPending);

	GetResult()->ReleaseMt();
}

ThreadPoolImpl::TaskImpl::Status ThreadPoolImpl::TaskImpl::GetStatus() const
{
	return Status(REFLEX_ATOMIC_READ(m_status));
}

Float ThreadPoolImpl::TaskImpl::GetProgress() const
{
	return REFLEX_ATOMIC_READ_UNORDERED(m_context.progress);
}

AlreadyRetained <Object> ThreadPoolImpl::TaskImpl::GetResult()
{
	return Cast<Object>(REFLEX_ATOMIC_READ(m_presult));
}

void ThreadPoolImpl::TaskImpl::Cancel()
{
	REFLEX_ATOMIC_WRITE(m_context.cancelled, true);
}

void ThreadPoolImpl::TaskImpl::Wait()
{
	//Pooled tasks have no dedicated thread to join. This can use a
	//condition variable when one is added to the System API.
	while (!Completed()) System::SuspendThread(25);
}

void ThreadPoolImpl::TaskImpl::Execute()
{
	if (m_context.Cancelled())
	{
		Complete({});
		return;
	}

	Complete(m_fn(m_context));
}

void ThreadPoolImpl::TaskImpl::CancelBeforeExecution()
{
	Cancel();
	Complete({});
}

void ThreadPoolImpl::TaskImpl::Complete(Worker::Result result)
{
	REFLEX_ASSERT(GetStatus() == kStatusPending);

	result.payload->RetainMt();

	REFLEX_ATOMIC_WRITE(m_presult, result.payload.Adr());

	Object::null.ReleaseMt();

	m_fn.Clear();

	REFLEX_ATOMIC_WRITE(m_status, result.success ? kStatusCompleted : kStatusFailed);
}

bool ThreadPoolImpl::TaskImpl::ContextImpl::Cancelled() const
{
	return REFLEX_ATOMIC_READ_UNORDERED(cancelled);
}

void ThreadPoolImpl::TaskImpl::ContextImpl::SetProgress(Float32 value_normalized)
{
	REFLEX_ATOMIC_WRITE_UNORDERED(progress, Clip(value_normalized, 0.0f, 1.0f));
}

ThreadPoolImpl::ThreadPoolImpl(UInt max_threads, System::Priority priority)
	: m_lock(System::CriticalSection::Create())
	, m_max_threads(Max<UInt>(max_threads, 1))
	, m_priority(priority)
{
}

ThreadPoolImpl::~ThreadPoolImpl()
{
	Shutdown();
}

Reference <Task> ThreadPoolImpl::Submit(const TaskFn & fn)
{
	REFLEX_ASSERT(m_threadvalidator);

	auto task = Make<ThreadPoolImpl::TaskImpl>(fn);

	UInt start_workers = 0;
	bool reject = false;

	{
		System::CriticalSection::Lock lock(m_lock);

		for (UInt idx = m_workers.GetSize(); idx;)
		{
			idx--;

			if (m_workers[idx]->Completed()) m_workers.Remove(idx);
		}

		if (m_stopping)
		{
			reject = true;
		}
		else
		{
			task->Attach(m_pending);

			start_workers = Min(m_pending.GetNumItem(), m_max_threads - m_active_workers);

			m_active_workers += start_workers;

			UpdateTaskCounts();
		}
	}

	if (reject)
	{
		task->CancelBeforeExecution();
	}
	else
	{
		Array <Reference<System::Thread>> workers;

		REFLEX_LOOP(idx, start_workers)
		{
			workers.Push(System::Thread::Create(BindMethod(*this, &ThreadPoolImpl::Worker), m_priority));
		}

		System::CriticalSection::Lock lock(m_lock);

		m_workers.Append(workers);
	}

	return task;
}

Pair <UInt> ThreadPoolImpl::GetNumTasks() const
{
	REFLEX_ASSERT(m_threadvalidator);

	auto counts = REFLEX_ATOMIC_READ_UNORDERED(m_task_counts);

	return { UInt16(counts), UInt16(counts >> 16) };
}

void ThreadPoolImpl::CancelPending()
{
	REFLEX_ASSERT(m_threadvalidator);

	ThreadPoolImpl::TaskImpl::List pending;

	{
		System::CriticalSection::Lock lock(m_lock);

		while (auto task = m_pending.GetFirst()) task->Attach(pending);

		UpdateTaskCounts();
	}

	while (auto task = pending.GetFirst())
	{
		Reference <Task> retain = *task;

		task->Detach();
		task->CancelBeforeExecution();
	}
}

void ThreadPoolImpl::Shutdown()
{
	REFLEX_ASSERT(m_threadvalidator);

	ThreadPoolImpl::TaskImpl::List pending;
	Array <Reference<System::Thread>> workers;

	{
		System::CriticalSection::Lock lock(m_lock);

		if (m_stopping) return;

		m_stopping = true;

		while (auto task = m_pending.GetFirst()) task->Attach(pending);

		for (auto & task : m_active) task.Cancel();

		UpdateTaskCounts();

		m_workers.Swap(workers);
	}

	while (auto task = pending.GetFirst())
	{
		Reference <Task> retain = *task;

		task->Detach();
		task->CancelBeforeExecution();
	}

	for (auto & worker : workers) worker->Wait();

	System::CriticalSection::Lock lock(m_lock);

	REFLEX_ASSERT(!m_active_workers);
	REFLEX_ASSERT(m_active.Empty());
}

void ThreadPoolImpl::Worker()
{
	for (;;)
	{
		Reference <Task> task;

		{
			System::CriticalSection::Lock lock(m_lock);

			if (m_stopping || m_pending.Empty())
			{
				m_active_workers--;
				return;
			}

			auto threadpool_task = m_pending.GetFirst();

			task = *threadpool_task;
			threadpool_task->Attach(m_active);

			UpdateTaskCounts();
		}

		Cast<ThreadPoolImpl::TaskImpl>(task)->Execute();

		{
			System::CriticalSection::Lock lock(m_lock);

			Cast<ThreadPoolImpl::TaskImpl>(task)->Detach();

			UpdateTaskCounts();
		}
	}
}

void ThreadPoolImpl::UpdateTaskCounts()
{
	auto pending = m_pending.GetNumItem();
	auto active = m_active.GetNumItem();

	REFLEX_ATOMIC_WRITE_UNORDERED(m_task_counts, UInt32(pending | (active << 16)));
}

NullThreadPool g_null_thread_pool;

REFLEX_END_INTERNAL

Reflex::Async::ThreadPool & Reflex::Async::ThreadPool::null = Reflex::Async::g_null_thread_pool;

Reflex::Unretained <Reflex::Async::ThreadPool> Reflex::Async::ThreadPool::Create(UInt max_threads, System::Priority priority, Allocator & allocator)
{
	return REFLEX_CREATE_EX(allocator, ThreadPoolImpl, max_threads, priority);
}

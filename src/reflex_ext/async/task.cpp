#include "../../../include/reflex_ext/async/task.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::Async)

struct NullAsyncTask : public Task
{
	Status GetStatus() const override { return kStatusFailed; }
	Float GetProgress() const override { return 0.0f; }
	TRef <Object> GetResult() override { return {}; }
	void Cancel() override {}
	void Wait() override {}
};

NullAsyncTask g_null_async_task;

REFLEX_END_INTERNAL

Reflex::Async::Task & Reflex::Async::Task::null = Reflex::Async::g_null_async_task;

Reflex::Async::Worker::Worker(const Function <void(Context & ctx)> & bg_task)
	: m_presult(&Object::null)
{
	Object::null.RetainMt();

	m_thread = System::Thread::Create([this, bg_task]()
	{
		bg_task(m_context);

		m_context.m_result_payload->RetainMt();		//this is an atomic increment
		
		REFLEX_ATOMIC_WRITE(m_presult, m_context.m_result_payload.Adr());	//copy to fg m_presult

		Object::null.ReleaseMt();	//balance counter on null object

		REFLEX_ATOMIC_WRITE(m_context.m_status, m_context.m_result_ok ? kStatusCompleted : kStatusFailed);

		REFLEX_ASSERT(m_context.m_result_set);
	});
}

Reflex::Async::Worker::~Worker()
{
	Cancel();

	m_thread->Wait();	//join

	GetResult()->ReleaseMt();
}

void Reflex::Async::Worker::Wait()
{
	m_thread->Wait();	//join
}

Reflex::TRef <Reflex::Async::Worker> Reflex::Async::Worker::Create(const Function <void(Context & ctx)> & bg_task)
{
	return Reflex::Detail::Constructor<Worker>::New(g_default_allocator, bg_task);
}

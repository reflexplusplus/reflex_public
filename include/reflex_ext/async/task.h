#pragma once

#include "[require].h"




//
//Primary API

namespace Reflex::Async
{

	class Task;				//abstract interface

	class Worker;			//default implementation handling a thread with runflag and progress, returning a result

}




//
//Async::Task

class Reflex::Async::Task : public System::Thread
{
public:

	REFLEX_OBJECT(Async::Task, System::Thread);

	static Task & null;

	enum Status : UInt8
	{
		kStatusPending,
		kStatusFailed,
		kStatusCompleted,
	};

	virtual Status GetStatus() const = 0;

	virtual Float GetProgress() const = 0;

	virtual TRef <Object> GetResult() = 0;

	virtual void Cancel() = 0;

	virtual void Wait() = 0;


	bool Completed() const final { return GetStatus() != kStatusPending; }

};




//
//Async::Worker

class Reflex::Async::Worker final : public Task		//!final, m_thread must be last member
{
public:
	
	//types
	
	struct Context
	{
		virtual bool Cancelled() const = 0;
		virtual void SetProgress(Float32 value_normalized) = 0;
	};

	struct Result
	{
		bool success = false;
		Reference <Object> payload;
	};



	//lifetime
	
	static TRef <Worker> Create(const Function <Result(Context & ctx)> & worker);

	~Worker();



	//access
	
	Float GetProgress() const override;

	Status GetStatus() const override;

	TRef <Object> GetResult() override;

	void Cancel() override;

	void Wait() override;



private:

	friend Reflex::Detail::Constructor <Worker>;

	Worker(const Function <Result(Context & ctx)> & worker);

	struct ContextImpl : public Context
	{
		bool Cancelled() const override;

		void SetProgress(Float32 value_normalized) override;

		AtomicFloat32 m_progress = 0.0f;
		AtomicUInt8 m_cancelled = false;
		AtomicUInt8 m_status = kStatusPending;
	};

	ContextImpl m_context;

	AtomicPointer m_presult;

	Reference <System::Thread> m_thread;	//!must be last

};




//
//impl

inline void Reflex::Async::Worker::Cancel()
{
	REFLEX_ATOMIC_WRITE(m_context.m_cancelled, true);
}

inline Reflex::Float Reflex::Async::Worker::GetProgress() const
{
	return REFLEX_ATOMIC_READ_UNORDERED(m_context.m_progress);
}

inline Reflex::Async::Task::Status Reflex::Async::Worker::GetStatus() const
{
	return Status(REFLEX_ATOMIC_READ(m_context.m_status));
}

inline Reflex::TRef <Reflex::Object> Reflex::Async::Worker::GetResult()
{
	return Cast<Object>(REFLEX_ATOMIC_READ(m_presult));
}

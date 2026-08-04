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
		bool Cancelled() const;

		void SetProgress(Float32 value_normalized);

		void SetResult(bool success, TRef <Object> payload = {});



	protected:

		friend Worker;

		AtomicFloat32 m_progress = 0.0f;
		
		AtomicUInt8 m_cancelled = false;
		AtomicUInt8 m_status = kStatusPending;

		bool m_result_ok = false;
		bool m_result_set = false;
	
		Reference <Object> m_result_payload;

	};



	//lifetime
	
	static TRef <Worker> Create(const Function <void(Context & ctx)> & worker);

	~Worker();



	//access
	
	Float GetProgress() const override;

	Status GetStatus() const override;

	TRef <Object> GetResult() override;

	void Cancel() override;

	void Wait() override;



private:

	friend Reflex::Detail::Constructor <Worker>;

	Worker(const Function <void(Context & ctx)> & worker);


	Context m_context;

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

inline bool Reflex::Async::Worker::Context::Cancelled() const
{
	return REFLEX_ATOMIC_READ_UNORDERED(m_cancelled);
}

inline void Reflex::Async::Worker::Context::SetProgress(Float value)
{
	REFLEX_ATOMIC_WRITE_UNORDERED(m_progress, value);
}

inline void Reflex::Async::Worker::Context::SetResult(bool success, TRef <Object> payload)
{
	m_result_ok = success;

	m_result_payload = payload;

	m_result_set = true;
}

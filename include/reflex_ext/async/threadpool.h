#pragma once

#include "task.h"




//
//Primary API

namespace Reflex::Async
{

	class ThreadPool;

}




//
//Async::ThreadPool

class Reflex::Async::ThreadPool : public Object
{
public:

	REFLEX_OBJECT(Async::ThreadPool, Object);

	static ThreadPool & null;



	//types

	using TaskFn = Function <Worker::Result(Worker::Context &)>;



	//lifetime

	[[nodiscard]] static Unretained <ThreadPool> Create(UInt max_threads, System::Priority priority = System::kPriorityBackground, Allocator & allocator = g_default_allocator);



	//tasks

	virtual Reference <Task> Submit(const TaskFn & task) = 0;

	virtual Pair <UInt> GetNumTasks() const = 0;	//approximate lock-free snapshot: pending, active

	virtual void CancelPending() = 0;



	//shutdown

	virtual void Shutdown() = 0;	//cancels queued/running tasks and waits for workers

};

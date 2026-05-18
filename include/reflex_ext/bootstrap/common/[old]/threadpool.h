#pragma once

#include "async_task.h"




//
//declarations

namespace Reflex::Bootstrap
{

	class ThreadPool;

}




//
//ThreadPool

class Reflex::Bootstrap::ThreadPool : public Object
{
public:

	//types

	class Job;



	//lifetime

	static TRef <ThreadPool> Create();



	//config

	virtual void SetMaxNumThreads(UInt n) = 0;

	virtual UInt GetMaxNumThreads() const = 0;



	//access

	virtual TRef <Job> CreateJob(const Function <bool(const bool & run, Float& progress)> & bgjob, const Data::Archive::View & clientdata = {}) = 0;

	virtual Pair <UInt> GetNumJobs() const = 0;	//pending,active

};




//
//ThreadPool::Job

class Reflex::Bootstrap::ThreadPool::Job : public Bootstrap::AsyncTask
{
public:

	static Job & null;

	virtual Data::Archive::View GetClientData() const = 0;

};

#pragma once

#include "thread.h"
#include "file_handle.h"




//
//Primary API

namespace Reflex::System
{

	class Process;

}




//
//Process

class Reflex::System::Process : public Thread
{
public:

	REFLEX_OBJECT(System::Process, Thread);

	static Process & null;


	struct Options
	{
		Priority priority = kPriorityNormal;
		FileHandle * std_out = nullptr;
		bool allow_window = true;
	};



	//lifetime

	[[nodiscard]] static TRef <Process> Create(const WString & path, ArrayView <WString> args, const Options & options);

	[[nodiscard]] static TRef <Process> Create(const WString & path, ArrayView <WString> args);	//clang workaroud



	//interface

	virtual Optional <Int32> GetExitCode() const = 0;

	virtual void Detach() = 0;

	virtual void Terminate() = 0;

};




//
//impl

inline Reflex::TRef <Reflex::System::Process> Reflex::System::Process::Create(const WString & path, ArrayView <WString> args)
{
	return Create(path, args, {});
}

#pragma once

#include "thread.h"




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



	//lifetime

	[[nodiscard]] static TRef <Process> Create(const WString & path, const ArrayView <WString::View> & args, Priority priority = kPriorityNormal, bool allow_window = true);



	//interface

	virtual bool Status() const = 0;

	virtual void Terminate() = 0;

};

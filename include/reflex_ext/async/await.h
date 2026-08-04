#pragma once

#include "task.h"
#include "periodic_clock.h"




//
//Primary API

namespace Reflex::Async
{

	void AttachAwait(Data::PropertySet & object, Key32 clock_id, TRef <Task> task, const Function <void(bool ok, Reflex::Object & result)> & callback);

	void AttachAwait(GLX::Object & object, Key32 clock_id, TRef <Task> task, const Function <void(bool ok, Reflex::Object & result)> & callback);

	void CancelAwait(Data::PropertySet & object, Key32 clock_id);

}




//
//impl

REFLEX_NS(Reflex::Async::Detail)

void AttachAwait(Data::PropertySet & object, Key32 clock_id, TRef <Task> task, decltype (&CreatePeriodicClock) create_clock, const Function <void(bool ok, Reflex::Object & result)> & callback);

REFLEX_END

inline void Reflex::Async::AttachAwait(Data::PropertySet & object, Key32 clock_id, TRef <Task> task, const Function <void(bool ok, Reflex::Object & result)> & callback)
{
	Detail::AttachAwait(object, clock_id, task, &CreatePeriodicClock, callback);
}

inline void Reflex::Async::CancelAwait(Data::PropertySet & object, Key32 clock_id)
{
	UnsetAbstractProperty(object, clock_id);
}
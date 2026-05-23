#pragma once

#include "../[require].h"




//
//impl

inline void Reflex::Async::AttachAwait(GLX::Object & object, Key32 id, TRef <Task> task, const Function <void(bool ok, Reflex::Object & result)> & callback)
{
	Detail::AttachAwait(object, id, task, &GLX::CreatePeriodicClock, callback);
}

#pragma once

#include "../object.h"




//
//Primary API

namespace Reflex::GLX
{

	TRef <Reflex::Object> CreateAnimationClock(const Function <void(Float32 delta)> & callback);

	TRef <Reflex::Object> CreatePeriodicClock(Float initial_delay, Float interval, const Function <void()> & callback);


	void AttachAnimationClock(Object & object, Key32 id, const Function <void(Float32 delta)> & callback);

	void AttachPeriodicClock(Object & object, Key32 id, Float initial_delay, Float interval, const Function <void()> & callback);


	void DetachClock(Object & object, Key32 id);							//helper for UnsetProperty<Reflex::Object>(object, id)

}




//
//imp

REFLEX_INLINE Reflex::TRef <Reflex::Object> Reflex::GLX::CreateAnimationClock(const Function <void(Float32 delta)> & callback)
{
	return Core::desktop->CreateAnimationClock(callback);
}

REFLEX_INLINE void Reflex::GLX::AttachAnimationClock(Object & object, Key32 id, const Function <void(Float32 delta)> & callback)
{
	SetAbstractProperty(object, id, CreateAnimationClock(callback));
}

REFLEX_INLINE void Reflex::GLX::AttachPeriodicClock(Object & object, Key32 id, Float initial_delay, Float interval, const Function <void()> & callback)
{
	SetAbstractProperty(object, id, CreatePeriodicClock(initial_delay, interval, callback));
}

inline void Reflex::GLX::DetachClock(Object & object, Key32 id)
{
	UnsetAbstractProperty(object, id);
}

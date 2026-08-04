#pragma once

#include "../object.h"




//
//Detail

REFLEX_NS(Reflex::GLX::Detail)

struct RecordedEvent;

void FlushEventLog(const Function <void(RecordedEvent &)> & callback);

REFLEX_END




//
//Detail::RecordedEvent

struct Reflex::GLX::Detail::RecordedEvent : public Reflex::Object
{
	struct MovableWeakRef
	{
		MovableWeakRef() = default;
		MovableWeakRef(const MovableWeakRef & rhs) = delete;
		MovableWeakRef(MovableWeakRef && rhs) { ref.Store(rhs.ref.Load()); }

		MovableWeakRef& operator=(const MovableWeakRef& rhs) = delete;
		MovableWeakRef & operator=(MovableWeakRef&& rhs) = delete;

		Reflex::Detail::WeakRef <GLX::Object> ref;
	};


	RecordedEvent(UInt uid, Key32 event_id, GLX::Object & object);

	const UInt uid;

	const Key32 event_id;

	const Reflex::Detail::WeakRef <GLX::Object> source;

	
	Array <MovableWeakRef> steps;
};




//
// impl

#if !REFLEX_DEBUG

inline void Reflex::GLX::Detail::FlushEventLog(const Function<void(RecordedEvent &)> &) {}

#endif

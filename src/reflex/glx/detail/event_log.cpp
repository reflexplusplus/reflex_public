#include "../../../../include/reflex/glx/detail/event_log.h"




#if REFLEX_DEBUG

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

REFLEX_INSTANTIATE_DEFAULT_ALLOCATOR;

constexpr UInt kMaxEvents = 16;

UInt g_recorded_event_count = 0;

Array < Reference <RecordedEvent> > g_recorded_events;

REFLEX_END_INTERNAL

Reflex::GLX::Detail::RecordedEvent::RecordedEvent(UInt uid, Key32 event_id, GLX::Object & object)
	: uid(uid)
	, event_id(event_id)
	, source(object)
{
}

void Reflex::GLX::Detail::LogEventStep(Object & src, Event & e, GLX::Object & receiver)
{
	//REFLEX_ASSERT(e.GetAllocator());

	REFLEX_RFOREACH(i, g_recorded_events)
	{
		if (i->event_id == e.id && i->source.Compare(&src))
		{
			i->steps.Push().ref.Store(receiver);

			return;
		}
	}

	if (g_recorded_events.GetSize() >= kMaxEvents)
	{
		g_recorded_events.Remove(0);
	}

	auto rec = New<RecordedEvent>(++g_recorded_event_count, e.id, src);

	rec->steps.Push().ref.Store(receiver);

	g_recorded_events.Push(rec);
}

void Reflex::GLX::Detail::FlushEventLog(const Function<void(RecordedEvent &)> & callback)
{
	for (auto & i : g_recorded_events)
	{
		callback(i);
	}

	g_recorded_events.Clear();
}

#endif

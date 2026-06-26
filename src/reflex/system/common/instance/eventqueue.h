#pragma once

#include "../[require].h"




//
//Internal

REFLEX_NS(Reflex::System::Common)

class EventInQueue
{
public:

	//lifetime

	EventInQueue();



	//write

	template <class TYPE> void Append(AudioPlugin::Event::Type type, UInt16 position, UInt16 idx, TYPE value);



	//read

	const AudioPlugin::EventBuffer & Flush(bool clock, Float64 bps, Float64 beatpos);



private:

	AudioPlugin::Event m_events[2][128];

	AtomicUInt32 m_state;	//[writepos, bufferidx]

	AudioPlugin::EventBuffer m_eventbuffer;

};

inline EventInQueue::EventInQueue()
	: m_state(0)
{
}

template <class TYPE> REFLEX_INLINE void EventInQueue::Append(AudioPlugin::Event::Type type, UInt16 position, UInt16 idx, TYPE value)
{
	REFLEX_STATIC_ASSERT(sizeof(TYPE) == 4);

	AudioPlugin::Event evnt = { type, position, idx };

	Reinterpret<UInt32>(evnt.value) = Reinterpret<UInt32>(value);

	auto [buffer_idx, write_pos] = Reinterpret<Pair<UInt16>>(REFLEX_ATOMIC_READ(m_state));

	auto buffer = m_events[buffer_idx] + write_pos;

	(*buffer) = evnt;

	auto state = Reinterpret<UInt32>(MakeTuple(buffer_idx, UInt16((write_pos + 1) & 255)));

	REFLEX_ATOMIC_WRITE(m_state, state);
}

REFLEX_INLINE const Reflex::System::AudioPlugin::EventBuffer & EventInQueue::Flush(bool clock, Float64 bps, Float64 beatpos)
{
	m_eventbuffer.clock = clock;

	m_eventbuffer.bps = bps;

	m_eventbuffer.beatpos = beatpos;

	auto [buffer_idx, write_pos] = Reinterpret<Pair<UInt16>>(REFLEX_ATOMIC_READ(m_state));

	m_eventbuffer.events = { m_events[buffer_idx], write_pos };

	auto state = Reinterpret<UInt32>(MakeTuple(UInt16((buffer_idx + 1) & 1), UInt16(0)));

	REFLEX_ATOMIC_WRITE(m_state, state);

	return m_eventbuffer;
}

REFLEX_END

#pragma once

#include "[include].h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::System::Common)

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

	UInt32 m_bufferidx;

	volatile UInt32 m_writepos;

	AudioPlugin::EventBuffer m_eventbuffer;

};

inline EventInQueue::EventInQueue()
	: m_writepos(0)
	, m_bufferidx(0)
{
}

template <class TYPE> REFLEX_INLINE void EventInQueue::Append(AudioPlugin::Event::Type type, UInt16 position, UInt16 idx, TYPE value)
{
	REFLEX_STATIC_ASSERT(sizeof(TYPE) == 4);

	AudioPlugin::Event evnt = { type, position, idx };

	Reinterpret<UInt32>(evnt.value) = Reinterpret<UInt32>(value);

	auto buffer = m_events[m_bufferidx] + m_writepos;

	(*buffer) = evnt;

	m_writepos = (m_writepos + 1) & 127;
}

REFLEX_INLINE const Reflex::System::AudioPlugin::EventBuffer & EventInQueue::Flush(bool clock, Float64 bps, Float64 beatpos)
{
	m_eventbuffer.clock = clock;

	m_eventbuffer.bps = bps;

	m_eventbuffer.beatpos = beatpos;

	m_eventbuffer.events = { m_events[m_bufferidx], UInt32(m_writepos) };

	m_writepos = 0;

	m_bufferidx ^= 1;	// + 1) & 3;

	return m_eventbuffer;
}

REFLEX_END_INTERNAL

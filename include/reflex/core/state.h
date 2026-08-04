#pragma once

#include "types.h"




//
//Primary API

namespace Reflex
{

	class State;

}




//
//State

class Reflex::State
{
public:

	//types

	class Monitor;



	//lifetime

	State();



	//info

	const UInt32 & GetChangeCount() const { return m_count; }



protected:

	State(const State &) {}


	//notify

	void Notify();



private:

	friend class Monitor;


	UInt m_count;

};




//
//State::Monitor

class Reflex::State::Monitor
{
public:

	//lifetime

	Monitor();

	Monitor(const State & state);

	Monitor(const Monitor & monitor);



	//setup

	void Connect(const State & state);

	void Reconnect();

	void Disconnect();

	bool Connected() const;



	//check

	bool Poll() const;



private:

	mutable UInt32 m_count;

	const UInt32 * m_state_counter;

};




//
//impl

REFLEX_INLINE Reflex::State::State()
	: m_count(0)
{
}

REFLEX_INLINE void Reflex::State::Notify()
{
	++m_count;
}

REFLEX_INLINE Reflex::State::Monitor::Monitor()
	: m_count(0)
	, m_state_counter(&m_count)
{
}

REFLEX_INLINE Reflex::State::Monitor::Monitor(const State & state)
	: m_count(0)
	, m_state_counter(&m_count)
{
	Connect(state);
}

REFLEX_INLINE Reflex::State::Monitor::Monitor(const Monitor & monitor)
	: m_count(0)
	, m_state_counter(&m_count)
{
	m_state_counter = monitor.m_state_counter;

	m_count = (*m_state_counter) - 1;
}

REFLEX_INLINE void Reflex::State::Monitor::Connect(const State & state)
{
	m_state_counter = &state.m_count;

	m_count = (*m_state_counter) - 1;
}

REFLEX_INLINE void Reflex::State::Monitor::Reconnect()
{
	m_count = (*m_state_counter) - 1;
}

REFLEX_INLINE void Reflex::State::Monitor::Disconnect()
{
	m_state_counter = &m_count;
}

REFLEX_INLINE bool Reflex::State::Monitor::Connected() const
{
	return m_state_counter != &m_count;
}

REFLEX_INLINE bool Reflex::State::Monitor::Poll() const
{
	return SetFiltered(m_count, *m_state_counter);
}

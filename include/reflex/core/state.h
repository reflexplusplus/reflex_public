#pragma once

#include "types.h"




//
//Primary API

namespace Reflex
{

	template <bool MT, bool LEGACY = false> class ChangeCount;

	
	using State = ChangeCount <false>;

	using StateMt = ChangeCount <true>;

}




//
//ChangeCount

template <bool MT, bool LEGACY>
class Reflex::ChangeCount
{
public:

	//types

	using Storage = ConditionalType<MT, AtomicUInt32, UInt32>;

	class Monitor;



	//lifetime

	ChangeCount();



	//info

	UInt32 GetCurrentCount() const;

	const Storage * GetAdr() const { return &m_count; }



protected:

	ChangeCount(const ChangeCount &) {}


	//notify

	void Notify();



private:

	friend class Monitor;


	Storage m_count;

};




//
//ChangeCount::Monitor

template <bool MT, bool LEGACY>
class Reflex::ChangeCount<MT,LEGACY>::Monitor
{
public:

	//lifetime

	Monitor();

	Monitor(const ChangeCount & state);

	Monitor(const Monitor & monitor);



	//setup

	void Connect(const ChangeCount & state);

	void Disconnect();

	bool Connected() const;


	void Invalidate();	//force next Poll to true



	//read

	bool Poll() const;



private:

	mutable UInt32 m_count;

	mutable Storage m_count_disconnected;

	const Storage * m_state_counter;

};




//
//impl

template <bool MT, bool LEGACY> REFLEX_INLINE Reflex::ChangeCount<MT,LEGACY>::ChangeCount()
	: m_count(0)
{
}

template <bool MT, bool LEGACY> REFLEX_INLINE Reflex::UInt32 Reflex::ChangeCount<MT,LEGACY>::GetCurrentCount() const
{
	if constexpr (MT)
	{
		return m_count.load(std::memory_order_acquire);
	}
	else
	{
		return m_count;
	}
}

template <bool MT, bool LEGACY> REFLEX_INLINE void Reflex::ChangeCount<MT,LEGACY>::Notify()
{
	if constexpr (MT)
	{
		m_count.fetch_add(1, std::memory_order_release);
	}
	else
	{
		++m_count;
	}
}

template <bool MT, bool LEGACY> REFLEX_INLINE Reflex::ChangeCount<MT,LEGACY>::Monitor::Monitor()
	: m_count(0)
	, m_count_disconnected(0)
	, m_state_counter(&m_count_disconnected)
{
}

template <bool MT, bool LEGACY> REFLEX_INLINE Reflex::ChangeCount<MT,LEGACY>::Monitor::Monitor(const ChangeCount & state)
	: m_count(0)
	, m_count_disconnected(0)
	, m_state_counter(&m_count_disconnected)
{
	Connect(state);
}

template <bool MT, bool LEGACY> REFLEX_INLINE Reflex::ChangeCount<MT,LEGACY>::Monitor::Monitor(const Monitor & monitor)
	: m_count(0)
	, m_count_disconnected(0)
	, m_state_counter(&m_count_disconnected)
{
	m_state_counter = monitor.m_state_counter;

	if constexpr (LEGACY)
	{
		Invalidate();
	}
	else
	{
		m_count = monitor.m_count;
	}
}

template <bool MT, bool LEGACY> REFLEX_INLINE void Reflex::ChangeCount<MT,LEGACY>::Monitor::Connect(const ChangeCount & state)
{
	m_state_counter = &state.m_count;

	if constexpr (LEGACY)
	{
		Invalidate();
	}
	else
	{
		m_count = state.GetCurrentCount();
	}
}

template <bool MT, bool LEGACY> REFLEX_INLINE void Reflex::ChangeCount<MT,LEGACY>::Monitor::Invalidate()
{
	if constexpr (MT)
	{
		m_count = m_state_counter->load(std::memory_order_acquire) - 1;
	}
	else
	{
		m_count = (*m_state_counter) - 1;
	}
}

template <bool MT, bool LEGACY> REFLEX_INLINE void Reflex::ChangeCount<MT,LEGACY>::Monitor::Disconnect()
{
	if constexpr (MT)
	{
		m_count_disconnected.store(m_count, std::memory_order_relaxed);
	}
	else
	{
		m_count_disconnected = m_count;
	}

	m_state_counter = &m_count_disconnected;
}

template <bool MT, bool LEGACY> REFLEX_INLINE bool Reflex::ChangeCount<MT,LEGACY>::Monitor::Connected() const
{
	return m_state_counter != &m_count_disconnected;
}

template <bool MT, bool LEGACY> REFLEX_INLINE bool Reflex::ChangeCount<MT,LEGACY>::Monitor::Poll() const
{
	if constexpr (MT)
	{
		return SetFiltered(m_count, m_state_counter->load(std::memory_order_acquire));
	}
	else
	{
		return SetFiltered(m_count, *m_state_counter);
	}
}

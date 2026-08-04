#pragma once

#include "meta/functions.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE, UInt SIZE> class Queue;

}




//
//Queue (SPSC)

template <class TYPE, Reflex::UInt SIZE>
class Reflex::Queue
{
public:

	//typdef

	using Type = TYPE;



	//lifetime

	Queue();



	//write

	bool Push(const TYPE & value);

	bool Push(TYPE && value);


	bool Pop(TYPE & value);

	bool Flush(TYPE & value);	//Pop last value

	void Flush();



protected:

	void SetSize(UInt size); //allow for VM varadic implementation (use SIZE=1 + Detail::Constructor::CreateVaradic)

	ArrayRegion <TYPE> GetData() { return { m_data, m_size }; }


	std::atomic_bool m_has_client[2] = { false, false };



private:

	static constexpr bool kIsFixedSize = SIZE != 1;

	static constexpr UInt kWrap = SIZE - 1;

	
	static bool Empty(UInt readpos, UInt writepos) { return readpos == writepos; }

	bool Full(UInt writepos, UInt readpos) const { if constexpr (kIsFixedSize) { return (writepos - readpos) == SIZE; } else { return (writepos - readpos) == m_size; } }

	TYPE & GetElement(UInt idx) { if constexpr (kIsFixedSize) { return m_data[idx & kWrap]; } else { return m_data[idx & m_wrap]; } }


	alignas(64) AtomicUInt32 m_writepos;

	alignas(64) AtomicUInt32 m_readpos;

	UInt m_size, m_wrap;

	TYPE m_data[SIZE];

};




//
//impl

template <class TYPE, Reflex::UInt SIZE> inline Reflex::Queue<TYPE,SIZE>::Queue()
	: m_writepos(0)
	, m_readpos(0)
{
	static_assert(SIZE > 0 && (SIZE & (SIZE - 1)) == 0, "Queue SIZE must be power of 2");
}

template <class TYPE, Reflex::UInt SIZE> inline void Reflex::Queue<TYPE,SIZE>::SetSize(UInt size)
{
	if constexpr (!kIsFixedSize)
	{
		//REFLEX_ASSERT((size > 0 && (size & (size - 1))));

		m_size = size;

		m_wrap = size - 1;
	}
	else
	{
		REFLEX_ASSERT(false);
	}
}

template <class TYPE, Reflex::UInt SIZE> inline bool Reflex::Queue<TYPE,SIZE>::Push(const TYPE & value)
{
	auto writepos = REFLEX_ATOMIC_READ_UNORDERED(m_writepos);

	auto readpos = REFLEX_ATOMIC_READ(m_readpos);

	if (Full(writepos, readpos)) return false;

	if constexpr (kIsFixedSize)
	{
		m_data[writepos & kWrap] = value;
	}
	else
	{
		m_data[writepos & m_wrap] = value;
	}

	REFLEX_ATOMIC_WRITE(m_writepos, writepos + 1);

	return true;
}

template <class TYPE, Reflex::UInt SIZE> inline bool Reflex::Queue<TYPE,SIZE>::Push(TYPE && value)
{
	auto writepos = REFLEX_ATOMIC_READ_UNORDERED(m_writepos);

	auto readpos = REFLEX_ATOMIC_READ(m_readpos);

	if (Full(writepos, readpos)) return false;

	GetElement(writepos) = std::move(value);

	REFLEX_ATOMIC_WRITE(m_writepos, writepos + 1);

	return true;
}

template <class TYPE, Reflex::UInt SIZE> inline bool Reflex::Queue<TYPE,SIZE>::Pop(TYPE & value)
{
	auto writepos = REFLEX_ATOMIC_READ(m_writepos);

	auto readpos = REFLEX_ATOMIC_READ_UNORDERED(m_readpos);

	if (Empty(writepos, readpos)) return false;

	value = GetElement(readpos);

	REFLEX_ATOMIC_WRITE(m_readpos, readpos + 1);

	return true;
}

template <class TYPE, Reflex::UInt SIZE> inline bool Reflex::Queue<TYPE,SIZE>::Flush(TYPE & value)
{
	auto writepos = REFLEX_ATOMIC_READ(m_writepos);

	auto readpos = REFLEX_ATOMIC_READ_UNORDERED(m_readpos);

	if (Empty(writepos, readpos)) return false;

	value = GetElement(writepos - 1);

	REFLEX_ATOMIC_WRITE(m_readpos, writepos);

	return true;
}

template <class TYPE, Reflex::UInt SIZE> REFLEX_INLINE void Reflex::Queue<TYPE,SIZE>::Flush()
{
	auto writepos = REFLEX_ATOMIC_READ(m_writepos);

	REFLEX_ATOMIC_WRITE(m_readpos, writepos);
}

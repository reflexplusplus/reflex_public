#pragma once

#include "meta/auxtypes.h"
#include "list.h"
#include "function.h"




//
//Primary API

namespace Reflex
{

	template <class ... TYPES> class Signal;

	template <class ... TYPES> class SignalComponent;

}




//
//Signal

template <class ... TYPES>
class Reflex::Signal
{
public:

	//info

	UInt GetNumListener() const;



protected:

	class Mute;

	class Listener;



	//lifetime

	Signal();



	//setup

	TRef <Object> CreateListener(const Function <void(Detail::ArgPassType <TYPES> ...)> & callback);



	//notify

	void Notify(Detail::ArgPassType <TYPES> ... vargs) const;

	void Emit(Detail::ArgPassType <TYPES> ... vargs) const;



	//debug info 

	bool DebugProbablyMuted() const { return m_count >= kMaxUInt16; }



private:

	mutable UInt m_count;

	Item<Listener, false>::List m_list;
};




//
//SignalComponent

template <class ... TYPES>
class Reflex::SignalComponent : public Signal <TYPES...>
{
public:

	using typename Signal<TYPES...>::Mute;

	using Signal<TYPES...>::CreateListener;

	using Signal<TYPES...>::Notify;
	using Signal<TYPES...>::Emit;

	using Signal<TYPES...>::DebugProbablyMuted;
};




//
//Signal::Mute

template <class ... TYPES>
class Reflex::Signal<TYPES...>::Mute
{
public:

	Mute(Signal & signal);

	~Mute();

	const TRef <Signal> signal;

};




//
//impl

template <class ... TYPES>
class Reflex::Signal<TYPES...>::Listener :
	public Item < Listener, false >,
	public Function <void(Detail::ArgPassType<TYPES>...)>
{
protected:

	friend Signal;


	using ItemBase = Item < Listener, false >;

	using FunctionType = Function <void(Detail::ArgPassType <TYPES> ...)>;

	using Signal = Reflex::Signal <TYPES...>;



	//bind

	Listener(Signal & signal, const FunctionType & fn);

	Listener(Listener && rhs) = delete;

	Listener(const Listener &) = delete;

	Listener operator=(const Listener &) = delete;


	using ItemBase::Attach;

	using ItemBase::Detach;

};

template <class ... TYPES> inline Reflex::Signal<TYPES...>::Signal()
	: m_count(0)
{
}

template <class ... TYPES> REFLEX_INLINE Reflex::TRef <Reflex::Object> Reflex::Signal<TYPES...>::CreateListener(const Function <void(Detail::ArgPassType <TYPES> ...)> & callback)
{
	return REFLEX_CREATE(Listener, *this, callback);
}

template <class ... TYPES> REFLEX_INLINE Reflex::UInt Reflex::Signal<TYPES...>::GetNumListener() const
{
	return m_list.GetNumItem();
}

template <class ... TYPES> inline void Reflex::Signal<TYPES...>::Emit(Detail::ArgPassType <TYPES> ... vargs) const
{
	if (!m_count++)
	{
		SafeIterate(RemoveConst(this)->m_list, [](Listener & listener, Detail::ArgPassType <TYPES> ... vargs)
		{
			REFLEX_ASSERT(listener.GetAllocator());

			if (listener.GetRetainCount() > 1)
			{
				listener(vargs...);
			}
		},
		vargs...);
	}

	m_count--;
}

template <class ... TYPES> inline void Reflex::Signal<TYPES...>::Notify(Detail::ArgPassType <TYPES> ... vargs) const
{
	if (!m_count++)
	{
		SafeIterate(RemoveConst(this)->m_list, [](Listener & listener, Detail::ArgPassType <TYPES> ... vargs)
		{
			REFLEX_ASSERT(listener.GetAllocator());

			if (listener.GetRetainCount() > 1)
			{
				listener(vargs...);
			}
		},
		vargs...);
	}

	m_count--;
}

template <class ... TYPES> inline Reflex::Signal<TYPES...>::Mute::Mute(Signal & signal)
	: signal(signal)
{
	signal.m_count += kMaxUInt16;
}

template <class ... TYPES> inline Reflex::Signal<TYPES...>::Mute::~Mute()
{
	signal->m_count -= kMaxUInt16;
}

template <class ... TYPES> inline Reflex::Signal<TYPES...>::Listener::Listener(Signal & signal, const FunctionType & fn)
	: FunctionType(Cast<typename FunctionType::Base>(fn))
{
	Attach(signal.m_list);
}

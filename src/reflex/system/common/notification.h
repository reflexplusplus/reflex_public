#pragma once

#include "[require].h"




//
//declarations

REFLEX_NS(Reflex::System::Common)

class Signal;

struct Notification;

REFLEX_END





//
//callback

struct Reflex::System::Common::Notification : public Item <Notification,false>
{
	Notification(Signal & signal, void * client, void(*fn)(void*));

	using Item <Notification,false>::Detach;


	void * m_client;

	void(*m_fn)(void*);
};




//
//signal

class Reflex::System::Common::Signal
{
public:

	//lifetime

	Signal();

	~Signal();



	//content

	Notification * Create(void * client, void(*fn)(void*));



	//call

	void Notify();



private:

	friend Notification;

	static void Null(void *){}


	Notification::List m_list;

	Notification m_null;

};




//
//

REFLEX_INLINE Reflex::System::Common::Notification::Notification(Signal & signal, void * client, void(*fn)(void *))
	: m_client(client)
	, m_fn(fn)
{
	Attach(signal.m_list);
}
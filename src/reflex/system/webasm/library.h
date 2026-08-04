#pragma once

#include "sdk.h"
#include "../common/notification.h"




//
//Library

REFLEX_NS(Reflex::System::WebASM) 

struct Library : public Object
{
public:

	Library();

	~Library();

	TRef <Object> CreateListener(Notification id, void * client, void(*callback)(void*)) { return m_signals[id].Create(client, callback); }

	void EmitClock() { m_signals[kNotificationClock].Notify(); }


private:

	Reference <Object> m_core;

	Common::Signal m_signals[kNumNotification];
};

typedef The <Library> TheLibrary;

REFLEX_END

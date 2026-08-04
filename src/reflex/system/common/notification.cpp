#include "notification.h"




//
//

Reflex::System::Common::Notification * Reflex::System::Common::Signal::Create(void * client, void(*fn)(void*))
{
	REFLEX_ASSERT_MAINTHREAD("System::Common::Signal::Create");

	return REFLEX_CREATE(Notification, *this, client, fn);
}

Reflex::System::Common::Signal::Signal()
	: m_null(*this, this, &Signal::Null)
{
	REFLEX_ASSERT_MAINTHREAD("System::Common::Signal::Signal");

	m_null.Detach();
}

Reflex::System::Common::Signal::~Signal()
{
	REFLEX_ASSERT_MAINTHREAD("System::Common::Signal::~Signal");
}

void Reflex::System::Common::Signal::Notify()
{
	auto itr = m_list.GetFirst();

	while (itr)
	{
		auto & callback = *itr;

		Retain(callback);

		(*callback.m_fn)(callback.m_client);

		itr = callback.GetNext();

		Release(callback);
	}
}


#pragma once

#include "[require].h"




//
//Primary API

namespace Reflex::Async
{

	TRef <Object> CreatePeriodicClock(Float32 interval, const Function <void()> & callback);

}




//
//Detail

REFLEX_NS(Reflex::Async::Detail)

class PeriodicClockItem : public Item <PeriodicClockItem,false>
{
public:

	REFLEX_OBJECT(PeriodicClockItem, Reflex::Object);

	PeriodicClockItem(Float interval, const Function <void()> & callback);

	void Process(Float32 delta);



private:

	Function <void()> m_callback;

	Float32 m_interval;

	Float32 m_remaining;
};

REFLEX_END

inline Reflex::TRef <Reflex::Object> Reflex::Async::CreatePeriodicClock(Float32 interval, const Function <void()> & callback)
{
	return New<Detail::PeriodicClockItem>(interval, callback);
}

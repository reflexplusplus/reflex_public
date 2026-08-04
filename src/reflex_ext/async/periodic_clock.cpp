#include "../../../include/reflex_ext/async/periodic_clock.h"
#include "../../../include/reflex_ext/async/clock.h"




REFLEX_BEGIN_INTERNAL(Reflex::Async)

struct PeriodicClock
{
	List < Detail::PeriodicClockItem, false > list;

	Reference <Object> clock;

	UInt8 count = 0;
};

Reflex::Detail::Module::Member <PeriodicClock> g_periodic_clock(module);

Float64 g_periodic_clock_z = 0.0;

REFLEX_END_INTERNAL

Reflex::Detail::Module Reflex::Async::module("Reflex::Async", Reflex::System::module);

Reflex::Async::Detail::PeriodicClockItem::PeriodicClockItem(Float interval, const Function <void()> & callback)
	: m_callback(callback)
	, m_interval(interval)
	, m_remaining(interval)
{
	REFLEX_ASSERT_MAINTHREAD("Async::Detail::PeriodicClockItem::PeriodicClockItem");

	auto & periodic_clock = *g_periodic_clock;

	Attach(periodic_clock.list);

	if (!periodic_clock.clock)
	{
		g_periodic_clock_z = System::GetElapsedTime();

		periodic_clock.clock = CreateClock(periodic_clock, [](PeriodicClock & self)
		{
			if (!(++self.count & 3))
			{
				auto delta = Float32(SetDelta(g_periodic_clock_z, System::GetElapsedTime()));

				SafeIterate(self.list, [](PeriodicClockItem & item, Float32 delta)
				{
					item.Process(delta);
				},
				delta);

				if (self.list.Empty()) self.clock.Clear();
			}
		});
	}
}

REFLEX_INLINE void Reflex::Async::Detail::PeriodicClockItem::Process(Float32 delta)
{
	m_remaining -= delta;

	if (m_remaining <= 0.0f)
	{
		m_remaining = m_interval;

		m_callback();
	}
}

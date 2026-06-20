#include "../../../include/reflex_ext/async/clock.h"




//
//impl

Reflex::TRef <Reflex::Object> Reflex::Async::CreateClock(const Function <void()> & callback)
{
	struct Wrapper : public Object
	{
		Wrapper(const Function <void()> & callback)
			: m_callback(callback),
			m_clock(CreateClock(this, [](Wrapper & self)
			{
				self.m_callback();
			}))
		{
		}

		const Function <void()> m_callback;

		Reference <Object> m_clock;
	};

	return REFLEX_CREATE(Wrapper, callback);
}

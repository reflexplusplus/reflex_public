#include "reflex/glx/functions/clock.h"




Reflex::TRef <Reflex::Object> Reflex::GLX::CreatePeriodicClock(Float interval, const Function <void()> & callback)
{
	return CreateAnimationClock([m_interval = interval, m_callback = callback, m_delta = interval](Float delta) mutable
	{
		m_delta -= delta;

		if (m_delta < 0.0f)
		{
			m_delta = m_interval + m_delta;

			m_callback();
		}
	});
}

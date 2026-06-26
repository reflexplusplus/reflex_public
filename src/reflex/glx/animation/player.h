#pragma once

#include "../../../../include/reflex/glx/animation/animation.h"




//
//player

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

struct GlobalPlayer : public ContainerAnimation
{
	static inline UInt32 st_nactive = 0;

	GlobalPlayer & Run()
	{
		if (!m_clock)
		{
			m_clock = Core::desktop->CreateAnimationClock([this](Float32 delta)
			{
				auto n = m_animations.GetSize();

				ArrayRegion <Reference> buffer = { Cast<Reference>(REFLEX_STACKALLOC(n * sizeof(Reference))), n };

				auto ptr = m_animations.GetData();

				for (auto & i : buffer) Reflex::Detail::Constructor<Reference>::Construct(&i, (*ptr++)->value);

				for (auto & i : buffer)
				{
					auto & scene = *i;

					if (scene.GetRetainCount() > 2)
					{
						if (!ContainerAnimation::Process(scene, delta))
						{
							m_animations.Unset(&scene);
						}
					}
					else
					{
						m_animations.Unset(&scene);
					}
				}

				REFLEX_RFOREACH(i, buffer) Reflex::Detail::Constructor<Reference>::Destruct(i);

				st_nactive = m_animations.GetSize();

				if (!m_animations) m_clock.Clear();
			});
		}

		return *this;
	}

	virtual void OnBegin() override {}

	virtual bool OnClock(Float delta) override { return true; }

	virtual void OnSkip() override {}


	Map <Animation*, Reference> m_animations;

	Reflex::Reference <Reflex::Object> m_clock;
};

REFLEX_END_INTERNAL


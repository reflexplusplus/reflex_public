#include "../../../../include/reflex_ext/glx/animation/multi.h"




//
//multi

Reflex::GLX::Multi::Multi()
{
}

void Reflex::GLX::Multi::OnBegin()
{
	for (auto & i : GetScenes()) ContainerAnimation::Reset(i);
}

bool Reflex::GLX::Multi::OnClock(Float delta)
{
	bool active = false;

	for (auto & i : GetScenes())
	{
		active = Or(active, ContainerAnimation::Process(i, delta));
	}

	return active;
}

void Reflex::GLX::Multi::OnSkip()
{
	for (auto & i : GetScenes()) ContainerAnimation::Process(i, 1.0f);
}

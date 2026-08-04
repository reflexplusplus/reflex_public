#include "../../../../include/reflex/glx/animation/container.h"




//
//container

Reflex::GLX::ContainerAnimation::ContainerAnimation()
	: m_first(&Animation::null)
{
}

void Reflex::GLX::ContainerAnimation::Clear()
{
	OnClear();

	m_scenes.Clear();

	m_first = &Animation::null;
}

void Reflex::GLX::ContainerAnimation::Add(Animation & scene)
{
	REFLEX_ASSERT(scene.GetAllocator());

	m_scenes.Push(scene);

	m_first = m_scenes.GetFirst().Adr();
}

void Reflex::GLX::ContainerAnimation::OnSetTarget(GLX::Object & object)
{
	for (auto & i : m_scenes) i->SetTarget(object);
}

#include "../../../../include/reflex_ext/glx/animation/playlist.h"




//
//playlist

Reflex::GLX::PlayList::PlayList()
	: m_loop(false),
	m_play(false)
{
}

void Reflex::GLX::PlayList::EnableLoop(bool enable)
{
	m_loop = enable;
}

void Reflex::GLX::PlayList::OnClear()
{
	m_current.Clear();
}

void Reflex::GLX::PlayList::OnBegin()
{
	if (auto scenes = GetScenes())
	{
		m_current = scenes.GetFirst();

		ContainerAnimation::Reset(m_current);
	}
}

bool Reflex::GLX::PlayList::OnClock(Float delta)
{
	if (ContainerAnimation::Process(m_current, delta))
	{
		return true;
	}
	else
	{
		auto scenes = GetScenes();

		REFLEX_LOOP(idx, scenes.size)
		{
			if (scenes[idx].Adr() == m_current.Adr())
			{
				if (idx == scenes.size - 1)
				{
					if (m_loop)
					{
						m_current = scenes.GetFirst();

						ContainerAnimation::Reset(m_current);

						return true;
					}
					else
					{
						m_current.Clear();

						return false;
					}
				}
				else
				{
					m_current = scenes[idx + 1];

					ContainerAnimation::Reset(m_current);

					return true;
				}
			}
		}

		m_current.Clear();

		return false;
	}
}

void Reflex::GLX::PlayList::OnSkip()
{
	AnimationScope scope(false);		//SKIP PROXY

	auto scenes = GetScenes();

	while (auto ptr = SearchValue(scenes, m_current))
	{
		ptr++;

		if (ptr < scenes.data + scenes.size)
		{
			m_current = *ptr;

			ContainerAnimation::Reset(m_current);
		}
		else
		{
			break;
		}
	}

	m_current.Clear();
}

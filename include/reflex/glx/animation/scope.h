#pragma once

#include "animation.h"




//
//Primary API

namespace Reflex::GLX
{

	class AnimationScope;

}




//
//AnimationScope

class Reflex::GLX::AnimationScope
{
public:

	AnimationScope(bool enable, bool force = false)
		: m_previous(st_enabled)
	{
		st_enabled = enable && (st_enabled || force);
	}

	~AnimationScope()
	{
		st_enabled = m_previous;
	}

	static bool IsEnabled() { return st_enabled; }



protected:

	bool m_previous;

	static bool st_enabled;
};

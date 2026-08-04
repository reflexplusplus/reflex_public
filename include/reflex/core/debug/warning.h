#pragma once

#include "[require].h"




//
//macros

#define REFLEX_DEBUG_WARN_SCOPE(FLAG, ENABLE) REFLEX_IF_DEBUG(const Reflex::WarnScope REFLEX_CONCATENATE(_warn_scope_, __COUNTER__)(FLAG, ENABLE))

#define REFLEX_DEBUG_WARN(OUTPUT, FLAG, TEST, ...) REFLEX_IF_DEBUG(if ((FLAG) && !(TEST)) OUTPUT.LogEx(Reflex::kLogWarning, {}, __VA_ARGS__, " [", REFLEX_STRINGIFY(FLAG), ']'))




//
//impl

REFLEX_NS(Reflex)

struct WarnScope
{
	WarnScope(bool & flag, bool enabled)
		: flag(flag)
		, previous(flag)
	{
		flag = enabled;
	}

	~WarnScope()
	{
		flag = previous;
	}

	bool & flag;

	const bool previous;
};

REFLEX_END

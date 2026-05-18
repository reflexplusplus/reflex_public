#pragma once

#include "[require].h"




//
//macros

#define REFLEX_DEBUG_WARN_SCOPE(FLAG, ENABLE) REFLEX_IF_DEBUG(const Reflex::WarnScope <ENABLE> REFLEX_CONCATENATE(_warn_scope_, __COUNTER__)(FLAG))

#define REFLEX_DEBUG_WARN(OUTPUT, FLAG, TEST, ...) REFLEX_IF_DEBUG(if ((FLAG) && !(TEST)) OUTPUT.LogEx(Reflex::kLogWarning, {}, __VA_ARGS__, " [", REFLEX_STRINGIFY(FLAG), ']'))




//
//impl

REFLEX_NS(Reflex)

template <bool ENABLE>
struct WarnScope
{
	WarnScope(bool & flag)
		: flag(flag)
		, previous(flag)
	{
		flag = ENABLE;
	}

	~WarnScope()
	{
		flag = previous;
	}

	bool & flag;

	const bool previous;
};

REFLEX_END

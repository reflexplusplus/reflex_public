#include "implementation.h"




//
//cstyle

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

ViewPortImpl::ComputedStyle::ComputedStyle(const Style & style)
	: body(*style.QuerySubStyle("body", &Style::null))
{
	REFLEX_LOOP(idx, 2)
	{
		auto barstyle = style.QuerySubStyle(kXY[idx], &Style::null);

		bars[idx] = barstyle;
	}
}

REFLEX_END_INTERNAL

#pragma once

#include "reflex/glx.h"




REFLEX_NS(Reflex::GLX)

inline bool OfferUp(Object & object, Event & e)
{
	auto clone = AutoRelease(e.Clone());

	return object.GetParent()->Emit(clone);
}

REFLEX_INLINE bool IsInverted(Object & object)
{
	return BitCheck(object.GetLayoutFlags(), Detail::kStandardLayoutInvert);
}

REFLEX_END
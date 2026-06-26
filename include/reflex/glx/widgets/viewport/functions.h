#pragma once

#include "viewport.h"
#include "../../functions/lookup.h"




//
//

namespace Reflex::GLX
{

	TRef <AbstractViewPort> GetContainingViewPort(Object & object);


	void SyncViewports(AbstractViewPort & source, AbstractViewPort & target, bool x, bool y);

	void UnsyncViewports(AbstractViewPort & target);


	void Zoom(ZoomArea & viewport, bool axis, Float time, Float vo, Float vr, Float pixel_padding = 0.0f);

	void Reveal(ZoomArea & viewport, bool axis, Float vo, Float vr, Float pixel_padding);

}




//
//impl

REFLEX_NS(Reflex::GLX::Detail)

void ClearZoomToggle(ZoomArea & viewport, AbstractViewBar & viewbar);

void ToggleZoom(ZoomArea & viewport, AbstractViewBar & viewbar, bool y);

REFLEX_END

inline Reflex::TRef <Reflex::GLX::AbstractViewPort> Reflex::GLX::GetContainingViewPort(Object & object)
{
	return QueryParentByType<AbstractViewPort>(object, &AbstractViewPort::null);
}

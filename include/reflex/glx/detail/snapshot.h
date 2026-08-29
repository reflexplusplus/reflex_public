#pragma once

#include "reflex/glx.h"




//
//declarations

REFLEX_NS(Reflex::GLX::Detail)

struct AbstractClone : public GLX::Object
{
	virtual void SetContent(Object & source) = 0;
};

[[nodiscard]] TRef <AbstractClone> CreateCloneView();

[[nodiscard]] TRef <AbstractClone> CreateSnapshot(bool antialias = false);

REFLEX_END

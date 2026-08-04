#pragma once

#include "../../../../include/reflex/glx/core/context.h"




//
//helpers for wrappers

REFLEX_NS(Reflex::GLX::Detail)

struct TransformScope
{
	TransformScope(Core::RenderContext & ctx, const System::fPoint & origin);

	TransformScope(Core::RenderContext & ctx, const System::fPoint & origin, const System::fSize & scale);

	~TransformScope();


	Core::RenderContext & ctx;

	const System::Renderer::Transform previous;
};

REFLEX_END

REFLEX_INLINE Reflex::GLX::Detail::TransformScope::TransformScope(Core::RenderContext & ctx, const System::fPoint & origin)
	: ctx(ctx),
	previous(ctx.transform)
{
	ctx.transform = TransformMatrix(ctx.transform, origin);
}

REFLEX_INLINE Reflex::GLX::Detail::TransformScope::TransformScope(Core::RenderContext & ctx, const System::fPoint & origin, const System::fSize & scale)
	: ctx(ctx),
	previous(ctx.transform)
{
	ctx.transform = TransformMatrix(ctx.transform, origin, scale);
}

REFLEX_INLINE Reflex::GLX::Detail::TransformScope::~TransformScope()
{
	ctx.transform = previous;
}

#pragma once

#include "../../defines.h"
#include "../bitmap.h"




//
//Detail

REFLEX_NS(Reflex::GLX::Detail)

template <class TYPE> ConstTRef <TYPE> Compile(const Style & style);


System::Renderer::Transform TransformMatrix(const System::Renderer::Transform & in, Point origin);

System::Renderer::Transform TransformMatrix(const System::Renderer::Transform & in, Point origin, Scale scale);


void ApplyClip(Core::RenderContext & ctx, const SIMD::IntV4 & clip_rect);

template <bool X, bool Y> SIMD::IntV4 TransformAndApplyClip(Core::RenderContext & ctx, const Rect & rect);


void Render(const System::Renderer::Transform &transform, const System::Renderer::Graphic & graphic, const Colour & colour, Point position, Scale scale);

void DrawFill(const System::Renderer::Transform & transform, const Colour & colour, const Rect & rect);

REFLEX_END




//
//impl

REFLEX_NS(Reflex::GLX::Detail)

extern const SIMD::BoolV4 kClipX, kClipY;

extern const SIMD::FloatV4 kRoundingToleranceV4;

extern ConstTRef <Graphic> g_solid_rectangle;

template <bool SCALE> REFLEX_INLINE System::Renderer::Transform TransformMatrixImpl(const System::Renderer::Transform & in, const System::Renderer::Transform & transform)
{
	System::Renderer::Transform ctx = in;

	ctx.origin += (transform.origin * ctx.scale);

	if constexpr (SCALE) ctx.scale *= transform.scale;

	return ctx;
}

template <bool X, bool Y> REFLEX_INLINE SIMD::IntV4 TransformClipImpl(const SIMD::IntV4 & current, const Rect & next_abs)
{
	REFLEX_STATIC_ASSERT(X || Y);

	auto [x1, y1] = next_abs.origin;// +kRoundingTolerance;

	auto frect_v4 = SIMD::FloatV4(x1, y1, x1, y1) + SIMD::FloatV4(0.0f, 0.0f, next_abs.size.w, next_abs.size.h) + kRoundingToleranceV4;

	SIMD::IntV4 min = SIMD::LoToHi(current, current);

	SIMD::IntV4 max = SIMD::HiToLo(current, current);

	auto irect_v4 = Reflex::Clip(SIMD::Truncate(frect_v4), min, max);

	if constexpr (X & Y)
	{
		return irect_v4;
	}
	else if constexpr (X)
	{
		return SIMD::Select(kClipX, irect_v4, current);
	}
	else if constexpr (Y)
	{
		return SIMD::Select(kClipY, irect_v4, current);
	}
}

REFLEX_INLINE void SetClipImpl(Core::RenderContext & ctx, const SIMD::IntV4 & clip_rect)
{
	REFLEX_ASSERT(Core::g_renderer->GetCurrent() == ctx.canvas || IsNull(Core::g_renderer));

	ctx.clip_rect = clip_rect;

	auto shift = SIMD::LoToHi(SIMD::kiZero, clip_rect);

	Reinterpret<System::iRect>(clip_rect - shift);

	Core::g_renderer->SetClip(Reinterpret<System::iRect>(clip_rect - shift));
}

REFLEX_END

template <class TYPE> REFLEX_INLINE Reflex::ConstTRef <TYPE> Reflex::GLX::Detail::Compile(const Style & style)
{
	return Data::Detail::AcquireProperty<TYPE>(RemoveConst(style), kComputedStyle, style);
}

REFLEX_INLINE void Reflex::GLX::Detail::Render(const System::Renderer::Transform & transform, const System::Renderer::Graphic & graphic, const Colour & colour, Point position, Scale scale)
{
	graphic.Render(TransformMatrix(transform, position, scale), colour);
}

REFLEX_INLINE void Reflex::GLX::Detail::DrawFill(const System::Renderer::Transform & transform, const Colour & colour, const Rect & rect)
{
	Render(transform, g_solid_rectangle, colour, rect.origin, rect.size);
}

REFLEX_INLINE Reflex::System::Renderer::Transform Reflex::GLX::Detail::TransformMatrix(const System::Renderer::Transform & in, Point origin)
{
	return TransformMatrixImpl<false>(in, { .origin = origin });
}

REFLEX_INLINE Reflex::System::Renderer::Transform Reflex::GLX::Detail::TransformMatrix(const System::Renderer::Transform & in, Point origin, Scale scale)
{
	return TransformMatrixImpl<true>(in, { .origin = origin, .scale = scale });
}

REFLEX_INLINE void Reflex::GLX::Detail::ApplyClip(Core::RenderContext & ctx, const SIMD::IntV4 & clip_rect)
{
	ctx.clip_rect = clip_rect;

	auto shift = SIMD::LoToHi(SIMD::kiZero, clip_rect);

	Core::g_renderer->SetClip(Reinterpret<System::iRect>(clip_rect - shift));
}

template <bool X, bool Y> REFLEX_INLINE Reflex::SIMD::IntV4 Reflex::GLX::Detail::TransformAndApplyClip(Core::RenderContext & ctx, const System::fRect & rect)
{
	const auto & current = ctx.transform;

	auto prev_clip = ctx.clip_rect;

	auto clip_rect = TransformClipImpl<X, Y>(ctx.clip_rect, { { (rect.origin * current.scale) + current.origin }, rect.size * current.scale });

	ApplyClip(ctx, clip_rect);

	return prev_clip;
}

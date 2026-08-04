#pragma once

#include "wrappers.h"




//
//Detail::Mask

REFLEX_NS(Reflex::GLX::Detail)

struct MaskLayer : public StandardLayersRenderer
{
	static inline const MaskLayer * st_current = nullptr;


	template <bool MASK> struct Scope;


	MaskLayer(Graphic & mask, Array <LayerWithState> & content, Size pixelsize, bool invert);

	void Render(const System::Renderer::Transform & transform, const Colour & colour) const override;


	const ConstReference <Graphic> m_mask;

	const bool m_invert;

	mutable SIMD::IntV4 m_prior_clip;

	mutable System::Renderer::Transform m_prior_transform;
};

template <bool MASK>
struct MaskLayer::Scope
{
	Scope(System::Renderer & renderer, const MaskLayer * next);

	~Scope();


	const TRef <System::Renderer> renderer;

	const MaskLayer * previous;
};

struct RenderTargetScope : public Core::RenderContext
{
	RenderTargetScope(Core::RenderContext & ctx, System::Renderer::Canvas & c, const Colour & bg)
		: Core::RenderContext({ .canvas = &c, .viewport = { {}, c.GetSize() } }),
		previous(ctx)
	{
		//REFLEX_ASSERT(IsValid(c));

		st_current = this;

		c.SetCurrent();

		scope.Init(Core::g_renderer, nullptr);

		ApplyClip(*this, { 0, 0, Core::RenderContext::viewport.size.w, Core::RenderContext::viewport.size.h });

		scope->renderer->Clear(bg);
	}

	~RenderTargetScope()
	{
		canvas->Flush();

		previous.canvas->SetCurrent();

		scope.Deinit();

		ApplyClip(previous, previous.clip_rect);

		st_current = &previous;
	}

	Core::RenderContext & previous;

	Reflex::Detail::Initialiser < MaskLayer::Scope <false> > scope;
};

constexpr UInt kMaxTextureSize = 8192;

constexpr UInt kMaxLayerDensity = 8;

REFLEX_END




//
//impl

inline Reflex::GLX::Detail::MaskLayer::MaskLayer(Graphic & mask, Array <LayerWithState> & content, Size pixelsize, bool invert)
	: StandardLayersRenderer(content, pixelsize),
	m_mask(mask),
	m_invert(invert)
{
}


inline void Reflex::GLX::Detail::MaskLayer::Render(const System::Renderer::Transform & transform, const Colour & colour) const
{
	Scope <true> scope(Core::g_renderer, this);

	auto ctx = *Core::RenderContext::st_current;

	m_prior_clip = ctx.clip_rect;

	m_prior_transform = transform;

	scope.renderer->SetMask(m_mask, transform, m_invert);

	DrawLayers(ctx, colour, layers);
}

template <bool MASK> inline Reflex::GLX::Detail::MaskLayer::Scope<MASK>::Scope(System::Renderer & renderer, const MaskLayer * next)
	: renderer(renderer)
	, previous(st_current)
{
	if constexpr (!MASK)
	{
		REFLEX_ASSERT(!next);

		if (st_current) renderer.ClearMask();
	}

	st_current = next;
}

template <bool MASK> inline Reflex::GLX::Detail::MaskLayer::Scope<MASK>::~Scope()
{
	if (auto prev = Scope::previous)
	{
		auto ctx = *Core::RenderContext::st_current;

		renderer->SetClip(ctx.viewport);

		renderer->SetMask(prev->m_mask, prev->m_prior_transform, prev->m_invert);

		if constexpr (MASK) ApplyClip(ctx, prev->m_prior_clip);
	}
	else if constexpr (MASK)
	{
		renderer->ClearMask();
	}

	st_current = Scope::previous;
}

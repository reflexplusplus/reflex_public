#include "renderer.h"
#include "layers/render.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

inline Colour PreMult(const Colour & colour)
{
	return { colour.r * colour.a, colour.g * colour.a, colour.b * colour.a, colour.a };
}

struct StandardRenderer : 
	public Core::Renderer, 
	public Countable <K32("Renderer")>
{
	StandardRenderer(GLX::Object & target, bool responsive, const ComputedStyleImpl & cstyle);

	~StandardRenderer()
	{
		cstyle.m_renderer = &Core::Renderer::null;

		cstyle.ReleaseSt();
	}

	const TRef <GLX::Object> m_owner;

	const ComputedStyleImpl & cstyle;
};

template <bool BG, bool FG>
struct InvisibleRendererWithState : public StandardRenderer
{
	REFLEX_OBJECT(InvisibleRendererWithState, Core::Renderer);

	InvisibleRendererWithState(const ComputedStyleImpl & cstyle, GLX::Object & object);

	void OnAccommodate(Size & contentsize) override;

	void OnAlign(const Size & size, Float & contenth) override;

	void Render(const System::Renderer::Transform & t, const Colour & c) const override {}


	const ArrayOfLayer & bg;

	const ArrayOfLayer & fg;

	Array <LayerWithState> m_bg, m_fg;
};

template <bool CLIPX, bool CLIPY, bool BGCOLOR, bool BG, bool FG>
struct VisibleRendererWithState : public InvisibleRendererWithState <BG,FG>
{
	REFLEX_OBJECT(VisibleRendererWithState, Core::Renderer);

	VisibleRendererWithState(const ComputedStyleImpl & cstyle, GLX::Object & object);

	void OnAlign(const Size & size, Float & contenth) override;

	void OnRedraw(Core::RenderContext & ctx) override;

	void Render(const System::Renderer::Transform & transform, const Colour & c) const override;


	ZSort * m_zsort;

	Size m_pixelsize;
};

struct CachedBitmap : public Object
{
	inline static UInt32 g_uid_hack = kHashSeed;
	
	Reference <System::Renderer::Canvas> bitmap;

	Reference <System::Renderer::Graphic> image;

	UInt8 alpha = kMaxUInt8;

	System::iSize size_z = { 0,0 };
};

struct CachedRendererState : public Countable <K32("CachedRenderer")>
{
	CachedRendererState(const ComputedStyleImpl & cstyle, GLX::Object & object, Key32 uid)
		: m_cachedbitmap(Data::Detail::AcquireProperty<CachedBitmap>(object, uid)),
		m_pad_origin(Reinterpret<Point>(cstyle.GetRenderPadding().near * Reflex::MakeSize(-1.0f))),
		m_pad_size(Sum(cstyle.GetRenderPadding())),
		m_opacity({ 1.0f,1.0f,1.0f,cstyle.m_opacity }),
		m_pixdensity(kRenderModes[cstyle.GetRender()].d),
		m_resize(true)
	{
	}

	void Resize(const ComputedStyleImpl & cstyle, Int canvas_density, GLX::Object & object);


	Reference <CachedBitmap> m_cachedbitmap;

	Point m_pad_origin;

	Size m_pad_size;

	Colour m_opacity;

	UInt16 m_pixdensity;

	bool m_resize;
};

template <bool BG, bool FG>
struct CachedRenderer : 
	public VisibleRendererWithState <false,false,false,BG,FG>,
	public CachedRendererState
{
	REFLEX_OBJECT(CachedRenderer, Core::Renderer);

	typedef VisibleRendererWithState <false,false,false,BG,FG> VisibleRendererWithState;


	CachedRenderer(const ComputedStyleImpl & cstyle, GLX::Object & object);

	void OnAlign(const Size & size, Float & contenth) override;

	void OnRedraw(Core::RenderContext & ctx) override;

	void Render(const System::Renderer::Transform & transform, const Colour & c) const override;

	void DoDraw(Core::RenderContext & ctx);


	Colour m_premult_bg_colour;
};

StandardRenderer::StandardRenderer(GLX::Object & target, bool responsive, const ComputedStyleImpl & cstyle)
	: Core::Renderer(responsive),
	m_owner(target),
	cstyle(cstyle)
{
	REFLEX_ASSERT(RemoveConst(cstyle).GetAllocator());

	cstyle.RetainSt();
}

template <bool BG, bool FG> InvisibleRendererWithState<BG,FG>::InvisibleRendererWithState(const ComputedStyleImpl & cstyle, GLX::Object & object)
	: StandardRenderer(object, false, cstyle),
	bg(cstyle.m_layers[0]),
	fg(cstyle.m_layers[1])
{
	UInt8 flags = (BG || FG) ? kMaxUInt8 : 0;

	if constexpr (BG) flags &= AllocateLayers(object, bg, m_bg);
	
	if constexpr (FG) flags &= AllocateLayers(object, fg, m_fg);

	if constexpr (BG || FG) RemoveConst(Renderer::responsive) = Not(flags & Layer::kOptimisationFlagNotResponsive);
}

template <bool BG, bool FG> void InvisibleRendererWithState<BG,FG>::OnAccommodate(Size & contentsize)
{
	//isresponsive = isresponsive || Renderer::responsive;

	if constexpr (BG) AccommodateLayers(m_bg, contentsize);

	if constexpr (FG) AccommodateLayers(m_fg, contentsize);
}

template <bool BG, bool FG> void InvisibleRendererWithState<BG,FG>::OnAlign(const Size & size, Float & contenth)
{
	if constexpr (BG) AlignLayers(m_bg, size, contenth);

	if constexpr (FG) AlignLayers(m_fg, size, contenth);
}

template <bool CLIPX, bool CLIPY, bool BGCOLOR, bool BG, bool FG> VisibleRendererWithState<CLIPX,CLIPY,BGCOLOR,BG,FG>::VisibleRendererWithState(const ComputedStyleImpl & cstyle, GLX::Object & object)
	: InvisibleRendererWithState<BG,FG>(cstyle, object),
	m_zsort(0)
{
}

template <bool CLIPX, bool CLIPY, bool BGCOLOR, bool BG, bool FG> void VisibleRendererWithState<CLIPX,CLIPY,BGCOLOR,BG,FG>::OnAlign(const Size & size, Float & contenth)
{
	typedef InvisibleRendererWithState <BG,FG> Base;

	Base::OnAlign(size, contenth);

	auto & object = *Base::m_owner;

	if (object.ChildrenHaveZIndex())
	{
		m_zsort = Data::Detail::AcquireProperty<ZSort>(object, K32("ZSort")).Adr();

		BuildZIndex(object, m_zsort->value);
	}
	else
	{
		m_zsort = 0;
	}
}

template <bool CLIPX, bool CLIPY, bool BGCOLOR, bool BG, bool FG> void VisibleRendererWithState<CLIPX,CLIPY,BGCOLOR,BG,FG>::OnRedraw(Core::RenderContext & ctx)
{
	typedef InvisibleRendererWithState <BG,FG> Base;

	if constexpr (BG || FG)
	{
		m_pixelsize = kNormal / ctx.transform.scale;

		if constexpr (BG) RedrawLayers(Base::m_bg, m_pixelsize);

		if constexpr (FG) RedrawLayers(Base::m_fg, m_pixelsize);
	}
}

template <bool CLIPX, bool CLIPY, bool BGCOLOR, bool BG, bool FG> void VisibleRendererWithState<CLIPX,CLIPY,BGCOLOR,BG,FG>::Render(const System::Renderer::Transform & transform, const Colour & c) const
{
	typedef InvisibleRendererWithState <BG,FG> Base;

	auto & object = *Base::m_owner;

	auto & rect = object.GetRect();

	auto & size = rect.size;

	auto & ctx = *Core::RenderContext::st_current;

	TransformScope scope(ctx, rect.origin, object.GetScale());

	ctx.transform.opacity *= Base::cstyle.m_opacity;

	ConditionalType <CLIPX || CLIPY, SIMD::IntV4, NullType> prev_clip;

	if constexpr (CLIPX || CLIPY)
	{
		prev_clip = TransformAndApplyClip<CLIPX, CLIPY>(ctx, { kOrigin, size });
	}

	if constexpr (BGCOLOR) DrawFill(ctx.transform, Base::cstyle.m_colours[0], {kOrigin, size});

	if constexpr (BG) DrawLayers(ctx, Base::cstyle.m_colours[1], Base::m_bg);

	if (m_zsort)
	{
		auto & sequence = m_zsort->value;

		REFLEX_ASSERT(sequence.GetSize() == object.GetNumItem());

		for (auto & i : sequence) i.value->Draw(ctx);
	}
	else
	{
		for (auto & i : object) i.Draw(ctx);
	}

	if constexpr (FG) DrawLayers(ctx, Base::cstyle.m_colours[1], Base::m_fg);

	if constexpr (CLIPX || CLIPY)
	{
		ApplyClip(ctx, prev_clip);
	}
}

template <bool BG, bool FG> CachedRenderer<BG,FG>::CachedRenderer(const ComputedStyleImpl & cstyle, GLX::Object & object)
	: VisibleRendererWithState(cstyle, object),
	CachedRendererState(cstyle, object, CachedBitmap::g_uid_hack),
	m_premult_bg_colour(PreMult(cstyle.GetBackgroundColour()))
{
	auto & cachedbitmap = *m_cachedbitmap;

	if (SetFiltered(cachedbitmap.alpha, UInt8(true)))
	{
		cachedbitmap.bitmap = Core::g_renderer->CreateBitmap(Reinterpret<bool>(cachedbitmap.alpha), true);

		cachedbitmap.size_z = { kMaxInt32, kMaxInt32 };
	}
}

template <bool BG, bool FG> void CachedRenderer<BG,FG>::OnAlign(const Size & size, Float & contenth)
{
	VisibleRendererWithState::OnAlign(size, contenth);

	m_opacity.a = VisibleRendererWithState::cstyle.m_opacity;	//for transition

	m_resize = true;
}

REFLEX_NOINLINE void CachedRendererState::Resize(const ComputedStyleImpl & cstyle, Int canvas_density, GLX::Object & object)
{
	auto fsize = object.GetRect().size + m_pad_size;

	Int mult = Truncate(RoundUpPow2(cstyle.GetScale(), 1.0f));	//Truncate(RoundUp(scale))

	Int pixdensity = Clip<Int>(m_pixdensity * mult, canvas_density, 8);

	System::iSize isize = { Truncate(fsize.w + Core::kRoundingTolerance) & 8191, Truncate(fsize.h + Core::kRoundingTolerance) & 8191 };

	isize = Max(isize, System::Renderer::Canvas::kMinValidSize);

	auto & cachedbitmap = *m_cachedbitmap;

	if (SetFiltered(cachedbitmap.size_z, isize))
	{
		cachedbitmap.bitmap->SetSize(isize, pixdensity);

		Pair <System::fRect> rects = { { {}, fsize }, kNormalRect };

		cachedbitmap.image = cachedbitmap.bitmap->CreateTextures({ &rects, 1 });
	}
}

template <bool BG, bool FG> void CachedRenderer<BG,FG>::OnRedraw(Core::RenderContext & ctx)
{
	auto & object = *VisibleRendererWithState::m_owner;

	if (SetFiltered(m_resize, false))
	{
		Resize(VisibleRendererWithState::cstyle, ctx.canvas->GetPixelDensity(), object);
	}

	RenderTargetScope scope(ctx, m_cachedbitmap->bitmap, m_premult_bg_colour);

	TransformScope t(scope, m_pad_origin * Reflex::MakeSize(-1.0f));

	DoDraw(scope);
}

template <bool BG, bool FG> void CachedRenderer<BG,FG>::Render(const System::Renderer::Transform & transform, const Colour & c) const
{
	auto & object = *VisibleRendererWithState::m_owner;

	auto & rect = object.GetRect();

	auto & size = rect.size;

	auto & image = *m_cachedbitmap->image;

	auto & origin = rect.origin;

	auto & scale = object.GetScale();

	image.Render(TransformMatrix(transform, origin + m_pad_origin, (size + m_pad_size) * scale), c * m_opacity);
}

template <bool BG, bool FG> REFLEX_INLINE void CachedRenderer<BG,FG>::DoDraw(Core::RenderContext & ctx)
{
	if constexpr (BG || FG)
	{
		VisibleRendererWithState::m_pixelsize = kNormal / ctx.transform.scale;

		if constexpr (BG) RedrawLayers(VisibleRendererWithState::m_bg, VisibleRendererWithState::m_pixelsize);

		if constexpr (FG) RedrawLayers(VisibleRendererWithState::m_fg, VisibleRendererWithState::m_pixelsize);
	}

	if constexpr (BG)
	{
		DrawLayers(ctx, VisibleRendererWithState::cstyle.m_colours[1], VisibleRendererWithState::m_bg);
	}

	auto & object = *VisibleRendererWithState::m_owner;

	if (object.ChildrenHaveZIndex())
	{
		auto zsort = Data::Detail::AcquireProperty<ZSort>(object, kNullKey);

		BuildZIndex(object, zsort->value);

		REFLEX_ASSERT(zsort->value.GetSize() == object.GetNumItem());

		for (auto & i : zsort->value) i.value->Draw(ctx);
	}
	else
	{
		for (auto & i : object) i.Draw(ctx);
	}

	if constexpr (FG)
	{
		DrawLayers(ctx, VisibleRendererWithState::cstyle.m_colours[1], VisibleRendererWithState::m_fg);
	}
}

REFLEX_END_INTERNAL

Reflex::UInt8 Reflex::GLX::Detail::AllocateLayers(Object & object, const ArrayOfLayer & layers, Array <LayerWithState> & dynamic)
{
	UInt8 flags = kMaxUInt8;

	dynamic.Allocate(layers.GetSize());

	for (auto & i : layers)
	{
		auto & layer = *i;

		auto state = layer.CreateState(object);

		dynamic.Push<kAllocateNone>({ &layer, state });

		flags &= layer.flags;
	}

	return flags;
}

Reflex::GLX::Detail::MorphRenderer::MorphRenderer(const ComputedStyleTransition & cstyle, GLX::Object & object, bool texture)
	: Core::Renderer(false),
	cstyle(cstyle),
	m_cstyle_transition(cstyle)
{
	auto mask = UInt8(texture ? ComputedStyleImpl::kRenderFlagRenderToTexture : 0);

	CachedBitmap::g_uid_hack = kfrom;

	m_a = CreateRendererBinder::Bind(Access(cstyle.from).m_renderflags | mask)(Access(cstyle.from), object);

	CachedBitmap::g_uid_hack = kto;
	
	m_b = CreateRendererBinder::Bind(Access(cstyle.to).m_renderflags | mask)(Access(cstyle.to), object);

	CachedBitmap::g_uid_hack = kHashSeed;
	
	RemoveConst(Renderer::responsive) = m_a->responsive || m_b->responsive;
}

void Reflex::GLX::Detail::MorphRenderer::OnAccommodate(Size & contentsize)
{
	m_a->OnAccommodate(contentsize);

	m_b->OnAccommodate(contentsize);
}

void Reflex::GLX::Detail::MorphRenderer::OnAlign(const Size & size, Float & contenth)
{
	m_a->OnAlign(size, contenth);

	m_b->OnAlign(size, contenth);
}

void Reflex::GLX::Detail::MorphRenderer::OnRedraw(Core::RenderContext & ctx)
{
	//auto x = m_cstyle_transition->transition.x;

	//auto & matrix = ctx.transform;

	//Float opacity = matrix.opacity;

	//matrix.opacity = Pow((1.0f - x), 0.5f);

	m_a->OnRedraw(ctx);

	//matrix.opacity = Pow(x, 0.5f);

	m_b->OnRedraw(ctx);

	//matrix.opacity = opacity;
}

void Reflex::GLX::Detail::MorphRenderer::Render(const System::Renderer::Transform & transform, const Colour & colour) const
{
	auto x = m_cstyle_transition->transition.x;

	auto & ctx = *Core::RenderContext::st_current;

	auto & matrix = ctx.transform;
	
	Float opacity = matrix.opacity;

	matrix.opacity = Pow((1.0f - x), 0.5f);

	m_a->Render(matrix, kWhite);

	matrix.opacity = Pow(x, 0.5f);

	m_b->Render(matrix, kWhite);

	matrix.opacity = opacity;
}

template <bool VISIBLE, bool CLIPX, bool CLIPY, bool BGCOLOR, bool BG, bool FG, bool MASK, bool TEXTURE> Reflex::TRef <Reflex::GLX::Core::Renderer> Reflex::GLX::Detail::CreateRenderer(const ComputedStyleImpl & cstyle, GLX::Object & object)
{
	if constexpr (VISIBLE & TEXTURE)
	{
		typedef CachedRenderer <BG,FG> Renderer;

		return REFLEX_CREATE(Renderer, cstyle, object);// return RecycleRenderer<Renderer>(cstyle, object);
	}
	else if constexpr (VISIBLE)
	{
		typedef VisibleRendererWithState <CLIPX,CLIPY,BGCOLOR,BG,FG> Renderer;

		return REFLEX_CREATE(Renderer, cstyle, object);// return RecycleRenderer<Renderer>(cstyle, object);
	}
	else
	{
		typedef InvisibleRendererWithState <BG,FG> Renderer;

		return REFLEX_CREATE(Renderer, cstyle, object);// RecycleRenderer<Renderer>(cstyle, object);
	}
}

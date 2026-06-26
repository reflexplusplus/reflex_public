#pragma once

#include "../../library.h"




//
//

REFLEX_NS(Reflex::GLX::Detail)

typedef System::Renderer::Graphic Graphic;

template <bool VISIBLE, bool CLIPX, bool CLIPY, bool BGCOLOR, bool BG, bool FG, bool MASK, bool TEXTURE> TRef <Core::Renderer> CreateRenderer(const ComputedStyleImpl & cstyle, GLX::Object & object);

REFLEX_TBINDER_8P(CreateRenderer);


class MorphRenderer : public Core::Renderer
{
public:

	REFLEX_OBJECT(MorphRenderer, Core::Renderer);

	MorphRenderer(const ComputedStyleTransition & cstyle, GLX::Object & object, bool rendertotexture = false);

	const ComputedStyleTransition & cstyle;


protected:

	virtual void OnAccommodate(Size & contentsize) override;

	virtual void OnAlign(const Size & size, Float & contenth) override;

	virtual void OnRedraw(Core::RenderContext & ctx) override;

	virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override;


	ConstReference <ComputedStyleTransition> m_cstyle_transition;

	Reference <Core::Renderer> m_a, m_b;
};

class CachedMorphRenderer : public MorphRenderer
{
public:

	REFLEX_OBJECT(CachedMorphRenderer, MorphRenderer);

	CachedMorphRenderer(const ComputedStyleTransition & cstyle, GLX::Object & object)
		: MorphRenderer(cstyle, object, true)
	{
	}
};


typedef ObjectOf < Sequence < UInt32, Core::WeakReference > > ZSort;


UInt8 AllocateLayers(Object & object, const ArrayOfLayer & layerlist, Array <LayerWithState> & layers);

void AccommodateLayers(Array <LayerWithState> & layers, Size & contentsize);

void AlignLayers(Array <LayerWithState> & layers, const Size & size, Float & contentheight);


struct Dummy { void operator()(){} };

void RedrawLayers(Array <LayerWithState> & layers, const Size & pixelsize);

void DrawLayers(Core::RenderContext & ctx, const Colour & colour, const Array <LayerWithState> & layers);


REFLEX_INLINE Float Parabolic(Float x) { return (2.0f - x) * x; }


REFLEX_INLINE void BuildZIndex(GLX::Object & parent, ZSort::ValueType & sort)
{
	sort.Clear();

	Pair <UInt16,UInt16> idx;

	for (auto & i : parent)
	{
		idx.b = UInt16(i.GetZIndex() + 128);

		sort.Insert(Reinterpret<UInt32>(idx), i);

		idx.a++;
	}
}

REFLEX_INLINE ZSort::ValueType BuildZIndex(GLX::Object & parent)
{
	ZSort::ValueType zsort;
	
	BuildZIndex(parent, zsort);
	
	return zsort;
}

REFLEX_END

REFLEX_INLINE void Reflex::GLX::Detail::AccommodateLayers(Array <LayerWithState> & dynamic, Size & contentsize)
{
	for (auto & i : dynamic)
	{
		Size max;
		
		i.a->Accommodate(i.b, max);

		contentsize = Max(contentsize, max);
	}
}

REFLEX_INLINE void Reflex::GLX::Detail::AlignLayers(Array <LayerWithState> & dynamic, const Size & size, Float & contentheight)
{
	for (auto & i : dynamic)
	{
		Float h = 0.0f;

		i.a->Align(i.b, size, h);

		contentheight = Max(contentheight, h);
	}
}

REFLEX_INLINE void Reflex::GLX::Detail::RedrawLayers(Array <LayerWithState> & layers, const Size & pixelsize)
{
	for (auto & [layer,state,graphic_ref] : layers)
	{
		graphic_ref = layer->Redraw(*state, pixelsize);
	}
}

REFLEX_INLINE void Reflex::GLX::Detail::DrawLayers(Core::RenderContext & ctx, const Colour & colour, const Array <LayerWithState> & layers)
{
	auto & transform = ctx.transform;

	for (auto & i : layers)
	{
		i.c->Render(transform, colour);
	}
}

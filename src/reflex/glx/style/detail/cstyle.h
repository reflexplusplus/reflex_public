#pragma once

#include "reflex/glx/style.h"
#include "reflex/glx/detail/resource.h"

#include "../transition.h"




//
//

REFLEX_NS(Reflex::GLX::Detail)

REFLEX_DECLARE_KEY32(true);
REFLEX_DECLARE_KEY32(antialias);
REFLEX_DECLARE_KEY32(density);
REFLEX_DECLARE_KEY32(size);
REFLEX_DECLARE_KEY32(render);

struct ReferenceProperty : public Reflex::Object
{
	ReferenceProperty(Key32 id) : id(id) {}

	Key32 id;
};

using LayerWithState = Tuple < const Layer*, Reference <Reflex::Object>, Reference <Graphic> >;

struct ComputedStyleImpl : public ComputedStyle
{
	enum RenderFlags : UInt8
	{
		kRenderFlagNone = 0,

		kRenderFlagVisible = MakeBit(0),
		kRenderFlagClipX = MakeBit(1),
		kRenderFlagClipY = MakeBit(2),

		kRenderFlagBackgroundColour = MakeBit(3),
		kRenderFlagBackgroundLayers = MakeBit(4),
		kRenderFlagForegroundLayers = MakeBit(5),
		//kCStyleRenderFlagMaskLayers = MakeBit(6),

		kRenderFlagRenderToTexture = MakeBit(7),
	};

	static void NullPropertySetter(Detail::ComputedStyleImpl &, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags) {}

	using ComputedStyle::ComputedStyle;

	TRef <Core::Renderer> CreateRenderer(GLX::Object & object) const override;

	TRef <ComputedStyle> Mutate(const ComputedStyle & cstyle) const override;

	using ComputedStyle::m_margins;
	using ComputedStyle::m_minmax;
	using ComputedStyle::m_opacity;
	using ComputedStyle::m_scale;
	using ComputedStyle::m_zindex;
	using ComputedStyle::m_clip;
	using ComputedStyle::m_render;
	using ComputedStyle::m_colours;

	using ComputedStyle::m_propertyflags;
	using ComputedStyle::m_layoutflags;
	using ComputedStyle::m_renderflags;
	using ComputedStyle::m_compile_renderflags;
	using ComputedStyle::m_layers;
	using ComputedStyle::m_renderer;
	using ComputedStyle::m_transition;
	using ComputedStyle::m_layersource;
	using ComputedStyle::m_height_ratio;
};

class ComputedStyleTransition : public ComputedStyleImpl
{
public:
	
	REFLEX_OBJECT(ComputedStyleTransition, ComputedStyleImpl);
	
	[[nodiscard]] static TRef <ComputedStyleTransition> Create(ConstTRef <ComputedStyle> from, ConstTRef <ComputedStyle> to, ConstTRef <StateTransition> transition);

	virtual void Morph() = 0;

	
	const ComputedStyleImpl & from;
	
	const ComputedStyleImpl & to;
	
	const StateTransition & transition;
	
	

protected:

	ComputedStyleTransition(ConstTRef <ComputedStyle> from, ConstTRef <ComputedStyle> to, ConstTRef <StateTransition> transition);

};

Array <CString::View> SplitCommaDelimited(const CString::View & string);

void InitialiseCStyleSetters(Map < Address, Pair<decltype(&ComputedStyleImpl::NullPropertySetter),UInt16> > & setters);

void CreateLayers(const Style & style, const ArrayOfLayerDesc & input, ArrayOfLayer & output);

void SetClip(ComputedStyleImpl & cstyle, bool x, bool y);

void SetRender(ComputedStyleImpl & cstyle, ComputedStyle::Render mode, const Margin & pad = {});	//1 = auto

REFLEX_INLINE ComputedStyleImpl & Access(ComputedStyle & self) { return Cast<ComputedStyleImpl>(self); }

REFLEX_INLINE const ComputedStyleImpl & Access(const ComputedStyle & self) { return Cast<ComputedStyleImpl>(self); }

extern Tuple < CString::View, UInt32, bool, UInt8 > kRenderModes[ComputedStyle::kNumRender];

extern ConstTRef <Style> st_current_style;	//for cstyle setter

REFLEX_END

REFLEX_SET_TRAIT(Reflex::GLX::Detail::ComputedStyleImpl, IsSingleThreadExclusive)
REFLEX_SET_TRAIT(Reflex::GLX::Detail::ComputedStyleTransition, IsSingleThreadExclusive)




//
//t

/*
 
 struct AbstractComputedStyle : public System::Renderer::Graphic
 {
	struct Class;

	const Schema & schema;
 
	AlignFn OnAlign;
	AccomodateFn OnAccmoodate;
	OnRedrawFn OnRedraw;
 
	UInt8 state[1];
 };
 
 struct AbstractComputedStyle::Class : public StaticItem <Class>
 {
	TRef <AbstractComputedStyle> Create(const Style & style, const Data::PropertySet & params);
 };
 
 
 */

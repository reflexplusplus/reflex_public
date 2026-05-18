#pragma once

#include "../style/style.h"
#include "layer.h"




//
//Detail

REFLEX_NS(Reflex::GLX::Detail)

class ComputedStyle;

using ArrayOfLayer = Array < ConstReference <Layer> >;


void SetReference(Data::PropertySet & properties, Key32 property_id, Key32 binding_id);

REFLEX_END




//
//Detail::ComputedStyle

class Reflex::GLX::Detail::ComputedStyle : public Reflex::Object
{
public:

	static ComputedStyle & null;


	enum PropertyFlags : UInt16
	{
		kPropertyFlagNone = 0,

		kPropertyFlagZIndex = 1 << 0,
		kPropertyFlagMargin = 1 << 1,
		kPropertyFlagPadding = 1 << 2,
		kPropertyFlagMin = 1 << 3,
		kPropertyFlagMax = 1 << 4,
		kPropertyFlagOpacity = 1 << 5,
		kPropertyFlagScale = 1 << 6,
		kPropertyFlagClipRenderDensity = 1 << 7,
		kPropertyFlagBackgroundLayers = 1 << 8,
		kPropertyFlagForegroundLayers = 1 << 9,
		kPropertyFlagBackgroundColor = 1 << 10,
		kPropertyFlagAspectRatio = 1 << 11,
	};

	enum Render : UInt8
	{
		kRenderAuto,
		kRenderTrue,
		kRenderX2,
		kRenderX4,
		kRenderX8,
		kRenderX16,
		kRenderFalse,//last to allow manual override of render to texture

		kNumRender,
	};



	//lifetime

	[[nodiscard]] static TRef <ComputedStyle> Create(const Style & root, const Data::PropertySet & properties);

	[[nodiscard]] static inline TRef <ComputedStyle> Create(const Style & style) { return Create(style, style); }

	[[nodiscard]] static TRef <ComputedStyle> Create(Size min, Size max); 	//for SetBounds

	[[nodiscard]] static TRef <ComputedStyle> Create(bool clipx, bool clipy); 	//for SetClip

	[[nodiscard]] static TRef <ComputedStyle> Create(Float scale, Float opacity, Render render); 	//for SetOpacity & SetMagnification



	//properties

	Int8 GetZIndex() const;

	Pair <bool> GetClip() const { return m_clip; }

	const Margin & GetMargin() const;

	const Margin & GetPadding() const;

	const Pair <Size> & GetMinMax() const;

	Float GetHeightRatio() const;

	Float GetScale() const;

	Float GetOpacity() const;

	const Colour & GetBackgroundColour() const;


	Render GetRender() const;

	const Margin & GetRenderPadding() const;


	Float GetTransitionTime() const;



	//flags

	UInt16 GetPropertyFlags() const { return m_propertyflags; }

	UInt8 GetLayoutFlags() const { return m_layoutflags; }



	//

	virtual TRef <ComputedStyle> Mutate(const ComputedStyle & b) const = 0;

	virtual TRef <GLX::Core::Renderer> CreateRenderer(GLX::Object & object) const = 0;



protected:

	ComputedStyle();


	Margin m_margins[3];	//margin, padding, render_pad (TODO automatically deduce render_pad from layers)

	Pair <Size> m_minmax;

	Float m_opacity;

	Float m_scale;

	Int8 m_zindex;

	Pair <bool> m_clip;

	Render m_render;

	Colour m_colours[2] = { kTransparent, kWhite };


	Float m_height_ratio;

	Float m_transition;

	UInt16 m_propertyflags;

	UInt8 m_layoutflags;

	UInt8 m_renderflags;

	bool m_compile_renderflags = false;

	UInt8 m_layermorph = false;

	mutable TRef <Core::Renderer> m_renderer;

	ArrayOfLayer m_layers[2];


	UIntNative m_layersource[2] = { 0,0 };
};

REFLEX_SET_TRAIT(Reflex::GLX::Detail::ComputedStyle, IsSingleThreadExclusive)




//
//impl

REFLEX_INLINE const Reflex::GLX::Margin & Reflex::GLX::Detail::ComputedStyle::GetMargin() const
{
	return m_margins[0];
}

REFLEX_INLINE const Reflex::GLX::Margin & Reflex::GLX::Detail::ComputedStyle::GetPadding() const
{
	return m_margins[1];
}

REFLEX_INLINE const Reflex::Pair <Reflex::GLX::Size> & Reflex::GLX::Detail::ComputedStyle::GetMinMax() const
{
	return m_minmax;
}

REFLEX_INLINE Reflex::Float Reflex::GLX::Detail::ComputedStyle::GetHeightRatio() const
{
	return m_height_ratio;
}

REFLEX_INLINE Reflex::Float Reflex::GLX::Detail::ComputedStyle::GetOpacity() const
{
	return m_opacity;
}

REFLEX_INLINE Reflex::Float Reflex::GLX::Detail::ComputedStyle::GetScale() const
{
	return m_scale;
}

REFLEX_INLINE Reflex::Int8 Reflex::GLX::Detail::ComputedStyle::GetZIndex() const
{
	return m_zindex;
}

REFLEX_INLINE const Reflex::GLX::Colour & Reflex::GLX::Detail::ComputedStyle::GetBackgroundColour() const
{
	return m_colours[0];
}

REFLEX_INLINE Reflex::GLX::Detail::ComputedStyle::Render Reflex::GLX::Detail::ComputedStyle::GetRender() const
{
	return m_render;
}

REFLEX_INLINE const Reflex::GLX::Margin & Reflex::GLX::Detail::ComputedStyle::GetRenderPadding() const
{
	return m_margins[2];
}

REFLEX_INLINE Reflex::Float Reflex::GLX::Detail::ComputedStyle::GetTransitionTime() const
{
	return m_transition;
}

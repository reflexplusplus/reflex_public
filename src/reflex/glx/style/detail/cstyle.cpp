#include "cstyle.h"
#include "../stylesheet.h"
#include "renderer.h"
#include "layers/wrappers.h"

#include "../../library.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

REFLEX_DECLARE_KEY32(scale);
REFLEX_DECLARE_KEY32(transition);

template <class UINT, class ENUM> inline void SetFlag(UINT & flags, ENUM flag)
{
	flags |= UINT(flag);
}

template <class UINT, class ENUM> inline void UnsetFlag(UINT & flags, ENUM flag)
{
	flags &= ~UINT(flag);
}

template <class UINT, class ENUM> inline void SetFlag(UINT & flags, ENUM flag, bool value)
{
	if (value)
	{
		SetFlag(flags, flag);
	}
	else
	{
		UnsetFlag(flags, flag);
	}
}

Float GetStylesheetScale(const Style & style)
{
	ConstTRef <StyleSheet> sheet;

	const Style * pstyle = &style;

	do
	{
		if (auto psheet = DynamicCast<StyleSheet>(*pstyle))
		{
			sheet = psheet;

			break;
		}

		pstyle = pstyle->GetParent();
	} 
	while (pstyle);

	return sheet->kScale;
}

void Copy(const ComputedStyle & from, ComputedStyle & to);

template <bool ZINDEX, bool MARGIN, bool PADDING, bool MIN, bool MAX, bool OPACITY, bool SCALE, bool RENDER> 
static void ApplyModifyBy(ComputedStyleImpl & self, const ComputedStyleImpl & mod);

REFLEX_TBINDER_8P(ApplyModifyBy);

struct CStyleTransitionImpl : public ComputedStyleTransition
{
	CStyleTransitionImpl(ConstTRef <ComputedStyle> from, ConstTRef <ComputedStyle> to, ConstTRef <StateTransition> x);

	CStyleTransitionImpl(const CStyleTransitionImpl & copy) = delete;
	
	~CStyleTransitionImpl();


	void Morph() override;

	TRef <ComputedStyle> Mutate(const ComputedStyle & cstyle) const override;

	TRef <Core::Renderer> CreateRenderer(GLX::Object & object) const override;


	template <bool ZINDEX, bool MARGIN, bool PADDING, bool MIN, bool MAX, bool OPACITY, bool SCALE, bool RENDER> static void ApplyMorph(ComputedStyleImpl & self, const ComputedStyleImpl & from, const ComputedStyleImpl & to, Float x);

	REFLEX_TBINDER_8P(ApplyMorph);

	REFLEX_INLINE static Margin InterpolateMargin(Float x, const Margin & from, const Margin & to)
	{
		return Reinterpret<Margin>(LinearInterpolate(x, Reinterpret<Colour>(from), Reinterpret<Colour>(to)));
	}
};

REFLEX_INLINE void Copy(const ComputedStyle & from, ComputedStyle & to)
{
	auto fromimpl = Cast<ComputedStyleImpl>(from);

	auto toimpl = Cast<ComputedStyleImpl>(to);

	auto psrc = fromimpl->m_margins + 0;

	auto pend = GetAdr(fromimpl->m_renderer);

	MemCopy(psrc, toimpl->m_margins + 0, ToUIntNative(pend) - ToUIntNative(psrc));

	REFLEX_LOOP(idx, 2) toimpl->m_layers[idx] = fromimpl->m_layers[idx];
}

REFLEX_INLINE Float ToFloat(Float scale, const CString::View & string)
{
	if (string)
	{
		if (string.GetFirst() == '@')
		{
			return Reflex::RoundNearest(ToFloat32(string) * scale);
		}
	}

	return ToFloat32(string);
}

REFLEX_INLINE Size ParseSizeScaled(Float scale, const CString::View & string, const Size & default_ = {})
{
	auto lines = SplitCommaDelimited(string);

	if (lines.GetSize() == 2)
	{
		return { ToFloat(scale, lines[0]), ToFloat(scale, lines[1]) };
	}
	else if (lines.GetSize() == 1)
	{
		Float value = ToFloat(scale, string);

		return { value, value };
	}

	return default_;
}

REFLEX_INLINE void SetMarginProperty(ComputedStyleImpl & self, UInt16 idx, const Margin & value)
{
	static constexpr Tuple <ComputedStyle::PropertyFlags, UInt8,bool> kFlags[3] =
	{
		{ ComputedStyle::kPropertyFlagMargin, 0, false },
		{ ComputedStyleImpl::kPropertyFlagPadding, UInt8(MakeBit(kStandardLayoutPadding)), false },
		{ ComputedStyle::kPropertyFlagNone, 0, true }
	};

	self.m_margins[idx] = value;

	auto flags = kFlags[idx];
	SetFlag(self.m_propertyflags, flags.a);
	SetFlag(self.m_layoutflags, flags.b);
	self.m_compile_renderflags |= flags.c;
}

REFLEX_INLINE void SetSizeProperty(ComputedStyleImpl & self, UInt16 idx, Size value)
{
	static constexpr Tuple <ComputedStyle::PropertyFlags, UInt8> kFlags[2] =
	{
		{ ComputedStyle::kPropertyFlagMin, 0  },
		{ ComputedStyleImpl::kPropertyFlagMax, UInt8(MakeBit(kStandardLayoutMaxOrScaledOrAspectRatio)) },
	};

	(&self.m_minmax.a)[idx] = value;

	auto flags = kFlags[idx];
	SetFlag(self.m_propertyflags, flags.a);
	SetFlag(self.m_layoutflags, flags.b);
}

//void SetRenderPad(ComputedStyleImpl & self, const Margin & margin)
//{
//	self.m_render_pad = margin;
//
//	self.m_compile_renderflags = true;
//}

void SetZIndex(ComputedStyleImpl & self, Int8 value)
{
	self.m_zindex = value;

	SetFlag(self.m_propertyflags, ComputedStyle::kPropertyFlagZIndex);
}

void SetOpacityProperty(ComputedStyleImpl & self, Float value)
{
	if (value < 1.0f)
	{
		self.m_opacity = Max(value, 0.0f);

		SetFlag(self.m_propertyflags, ComputedStyle::kPropertyFlagOpacity);
	}
	else
	{
		self.m_opacity = 1.0f;

		UnsetFlag(self.m_propertyflags, ComputedStyle::kPropertyFlagOpacity);
	}

	self.m_compile_renderflags = true;
}

void SetScaleProperty(ComputedStyleImpl & self, Float value)
{
	if (value != 1.0f)
	{
		self.m_scale = Max(value, 0.0001f);

		SetFlag(self.m_propertyflags, ComputedStyle::kPropertyFlagScale);
		SetFlag(self.m_layoutflags, MakeBit(kStandardLayoutMaxOrScaledOrAspectRatio));
	}
	else
	{
		constexpr auto kMask = UInt16(ComputedStyle::kPropertyFlagMax | ComputedStyle::kPropertyFlagAspectRatio);

		self.m_scale = 1.0f;

		UnsetFlag(self.m_propertyflags, ComputedStyle::kPropertyFlagScale);
		SetFlag(self.m_layoutflags, MakeBit(kStandardLayoutMaxOrScaledOrAspectRatio), True(self.m_propertyflags & kMask));
	}

	self.m_compile_renderflags = true;
}

void SetAspectRatio(ComputedStyleImpl & self, Float value)
{
	self.m_height_ratio = 1.0f / Max(value, 0.0001f);

	SetFlag(self.m_propertyflags, ComputedStyle::kPropertyFlagAspectRatio);
	SetFlag(self.m_layoutflags, MakeBit(kStandardLayoutMaxOrScaledOrAspectRatio));
}

void SetTransition(ComputedStyleImpl & self, Float value)
{
	self.m_transition = value;
}

REFLEX_INLINE void Compile(ComputedStyleImpl & self)
{
	constexpr auto kMask = UInt16(ComputedStyle::kPropertyFlagOpacity | ComputedStyle::kPropertyFlagScale);

	if (SetFiltered(self.m_compile_renderflags, false))
	{
		//should be opacity or scale
		//opacity needs autodetection of padding

		bool autorender = True(self.m_propertyflags & kMask);

		REFLEX_ASSERT(autorender == True(self.m_propertyflags & (ComputedStyle::kPropertyFlagOpacity | ComputedStyle::kPropertyFlagScale)));

		kRenderModes[ComputedStyle::kRenderAuto].c = autorender;

		SetFlag(self.m_renderflags, ComputedStyleImpl::kRenderFlagVisible, True(self.m_opacity));
		SetFlag(self.m_renderflags, ComputedStyleImpl::kRenderFlagClipX, self.m_clip.a);
		SetFlag(self.m_renderflags, ComputedStyleImpl::kRenderFlagClipY, self.m_clip.b);
		SetFlag(self.m_renderflags, ComputedStyleImpl::kRenderFlagRenderToTexture, kRenderModes[self.m_render].c);
	}
}

template <bool ZINDEX, bool MARGIN, bool PADDING, bool MIN, bool MAX, bool OPACITY, bool SCALE, bool RENDER> void ApplyModifyBy(ComputedStyleImpl & self, const ComputedStyleImpl & mod)
{
	//TODO this can be optimised, dont need full Set functions because dont need to update hasproperty
	//instead of this TBIND function, have 4 different impls for the Morph function

	if constexpr (ZINDEX) SetZIndex(self, mod.m_zindex);

	if constexpr (MARGIN) SetMarginProperty(self, 0, mod.m_margins[0]);
	if constexpr (PADDING) SetMarginProperty(self, 1, mod.m_margins[1]);

	if constexpr (MIN) SetSizeProperty(self, 0, Max(self.m_minmax.a, mod.m_minmax.a));
	if constexpr (MAX) SetSizeProperty(self, 1, Min(self.m_minmax.b, mod.m_minmax.b));

	if constexpr (OPACITY) SetOpacityProperty(self, self.m_opacity * mod.m_opacity);

	if constexpr (SCALE) SetScaleProperty(self, self.m_scale * mod.m_scale);

	if constexpr (RENDER)
	{
		self.m_clip.a = self.m_clip.a || mod.m_clip.a;

		self.m_clip.b = self.m_clip.b || mod.m_clip.b;

		self.m_render = Max(self.m_render, mod.m_render);

		auto & render_pad = self.m_margins[2];
		auto & mod_render_pad = mod.m_margins[2];

		render_pad.near = Max(render_pad.near, mod_render_pad.near);
		render_pad.far = Max(render_pad.far, mod_render_pad.far);

		self.m_compile_renderflags = true;
	}

	self.m_colours[1] = self.m_colours[1] * mod.m_colours[1];

	if constexpr (ZINDEX|MARGIN|PADDING|MIN|MAX|OPACITY|SCALE|RENDER) Compile(self);
}

CStyleTransitionImpl::CStyleTransitionImpl(ConstTRef <ComputedStyle> fromx, ConstTRef <ComputedStyle> tox, ConstTRef <StateTransition> x)
	: ComputedStyleTransition::ComputedStyleTransition(fromx, tox, x)
{
	REFLEX_ASSERT(&from != &to);


	Copy(from, *this);

	auto & to = ComputedStyleTransition::to;

	m_propertyflags = m_propertyflags | to.m_propertyflags;


	m_render = Max(m_render, to.m_render);

	auto & render_pad = m_margins[2];
	auto & to_render_pad = to.m_margins[2];

	render_pad.near = Max(render_pad.near, to_render_pad.near);
	render_pad.far = Max(render_pad.far, to_render_pad.far);


	m_layoutflags |= to.m_layoutflags;

	m_renderflags |= to.m_renderflags;


	m_layermorph = 0;

	REFLEX_LOOP(fg, 2)
	{
		if (from.m_layersource[fg] != to.m_layersource[fg])
		{
			m_layermorph = 1 + (kCanRenderToTexture && m_render != kRenderFalse);

			break;
		}
	}
}

CStyleTransitionImpl::~CStyleTransitionImpl()
{
	Release(transition);

	Release(from);

	Release(to);
}

void CStyleTransitionImpl::Morph()
{
	auto x = ComputedStyleTransition::transition.x;

	ApplyMorphBinder::Bind(UInt8(m_propertyflags))(*this, from, to, x);

	m_colours[0] = LinearInterpolate(x, from.m_colours[0], to.m_colours[0]);
	m_colours[1] = LinearInterpolate(x, from.m_colours[1], to.m_colours[1]);
}

TRef <ComputedStyle> CStyleTransitionImpl::Mutate(const ComputedStyle & previous_base) const
{
	//special handler for this which will always be last mod

	auto previous = Cast<ComputedStyleImpl>(previous_base);

	if (&to != previous.Adr())	//when no mods present they can be the same
	{
		ApplyModifyByBinder::Bind(UInt8(previous->m_propertyflags))(RemoveConst(*this), previous);
	}

	return RemoveConst(this);
}

TRef <Core::Renderer> CStyleTransitionImpl::CreateRenderer(GLX::Object & object) const
{
#if REFLEX_DEBUG
	auto flags = m_renderflags;
	SetFlag(flags, ComputedStyleImpl::kRenderFlagVisible, True(m_opacity));
	SetFlag(flags, ComputedStyleImpl::kRenderFlagClipX, m_clip.a);
	SetFlag(flags, ComputedStyleImpl::kRenderFlagClipY, m_clip.b);
	REFLEX_ASSERT(flags == m_renderflags);
#endif

	switch (m_layermorph)
	{
	case 2:
		return REFLEX_CREATE(CachedMorphRenderer, *this, object);// &RecycleRenderer<CachedMorphRenderer>(*this, object);

	case 1:
		return REFLEX_CREATE(MorphRenderer, *this, object);// &RecycleRenderer<MorphRenderer>(*this, object);

	default:
		return CreateRendererBinder::Bind(m_renderflags & kCreateRendererFlags)(*this, object);
	}
}

template <bool ZINDEX, bool MARGIN, bool PADDING, bool MIN, bool MAX, bool OPACITY, bool SCALE, bool RENDER> void CStyleTransitionImpl::ApplyMorph(ComputedStyleImpl & self, const ComputedStyleImpl & from, const ComputedStyleImpl & to, Float x)
{
	if constexpr (ZINDEX) SetZIndex(self, to.m_zindex);

	if constexpr (MARGIN) SetMarginProperty(self, 0, InterpolateMargin(x, from.m_margins[0], to.m_margins[0]));
	if constexpr (PADDING) SetMarginProperty(self, 1, InterpolateMargin(x, from.m_margins[1], to.m_margins[1]));

	if constexpr (MIN) SetSizeProperty(self, 0, LinearInterpolate(x, from.m_minmax.a, to.m_minmax.a));
	if constexpr (MAX) SetSizeProperty(self, 1, LinearInterpolate(x, from.m_minmax.b, to.m_minmax.b));

	if constexpr (OPACITY)
	{
		SetOpacityProperty(self, LinearInterpolate(x, from.m_opacity, to.m_opacity));

		SetFlag(self.m_propertyflags, ComputedStyle::kPropertyFlagOpacity);	//SetOpacityProperty can disable the property if it starts at 1.0
	}

	if constexpr (SCALE)
	{
		SetScaleProperty(self, LinearInterpolate(x, from.m_scale, to.m_scale));

		SetFlag(self.m_propertyflags, ComputedStyle::kPropertyFlagScale);	//SetScale can disable if it starts at 1.0
	}

	//self.m_colour = LinearInterpolate(x, from.m_colour, to.m_colour);

	if constexpr (ZINDEX | MARGIN | PADDING | MIN | MAX | OPACITY | SCALE | RENDER) Compile(self);
}

template <auto FN>
struct FloatSetter
{
	static void FromFloat(ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
	{
		FN(self, Cast<Data::Float32Property>(object)->value);
	}

	static void FromFloats(ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
	{
		FN(self, Cast<Data::ArrayOfFloat32Property>(object)->value.GetFirst());
	}
};

//template <auto FN>
//struct SizeSetter
//{
//	static void FromSize(ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
//	{
//		FN(self, Cast<SizeProperty>(object)->value);
//	}
//
//	static void FromFloats(ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
//	{
//		FN(self, ToSize(Cast<Data::ArrayOfFloat32Property>(object)->value));
//	}
//
//	static void FromString(ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
//	{
//		FN(self, ParseSizeScaled(GetStylesheetScale(st_current_style), Cast<Data::CStringProperty>(object)->value));
//	}
//};

//template <auto FN>
//struct MarginSetter
//{
//	static void FromMargin(ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
//	{
//		FN(self, Cast<MarginProperty>(object)->value);
//	}
//
//	static void FromFloats(ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
//	{
//		FN(self, ToMargin(Cast<Data::ArrayOfFloat32Property>(object)->value, stylesheet_flags));
//	}
//
//	static void FromString(ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
//	{
//		FN(self, ParseMarginScaled(GetStylesheetScale(st_current_style), Cast<Data::CStringProperty>(object)->value));
//	}
//};

Pair <decltype(&ComputedStyleImpl::NullPropertySetter), UInt16> g_null_property_setter = { &ComputedStyleImpl::NullPropertySetter, 0 };

REFLEX_END_INTERNAL

Reflex::ConstTRef <Reflex::GLX::Style> Reflex::GLX::Detail::st_current_style;

Reflex::Tuple < Reflex::CString::View, Reflex::UInt32, bool, Reflex::UInt8 > Reflex::GLX::Detail::kRenderModes[ComputedStyle::kNumRender] =
{
	{ "auto", K32("auto"), false, 1 },
	{ "true", K32("true"), true, 1 },
	{ "x2", K32("x2"), true, 2 },
	{ "x4", K32("x4"), true, 4 },
	{ "x8", K32("x8"), true, 8 },
	{ "x16", K32("x16"), true, 16 },
	{ "false", K32("false"), false, 1 },
};

REFLEX_NOINLINE Reflex::Array <Reflex::CString::View> Reflex::GLX::Detail::SplitCommaDelimited(const CString::View & string)
{
	return Reflex::Split(string, ',');
}

Reflex::GLX::Detail::ComputedStyle::ComputedStyle()
	: m_minmax({ {}, kLarge })
	, m_opacity(1.0f)
	, m_scale(1.0)
	, m_zindex(0)
	, m_render(kRenderAuto)
	, m_height_ratio(0.0f)
	, m_transition(0.0f)
	, m_propertyflags(0)
	, m_layoutflags(0)
	, m_renderflags(ComputedStyleImpl::kRenderFlagVisible)
{
}

Reflex::TRef <Reflex::GLX::Detail::ComputedStyle> Reflex::GLX::Detail::ComputedStyle::Create(const Style & style, const Data::PropertySet & properties)
{
	REFLEX_STATIC_ASSERT(sizeof(Pair<UInt8>) == sizeof(UInt16));

	constexpr Pair <UInt8> kNonVirtual = { 1, 1 };

	auto self = REFLEX_CREATE(ComputedStyleImpl);

	st_current_style = style;

	auto & setters = g_library->m_cstylesetters;
	
	auto stylesheet_flags = style.stylesheet_flags;

	if (*Reinterpret<UInt16>(&Cast<StyleAccessor>(style)->m_is_root_style) == Reinterpret<UInt16>(kNonVirtual))
	{
		for (auto & i : properties.Iterate())	//fast way, owns all properties
		{
			auto adr = i.key;

			auto [fn, idx] = *setters.Search(adr, &g_null_property_setter);

			fn(*self, i.value, idx, stylesheet_flags);
		}
	}
	else
	{
		for (auto & i : setters)
		{
			auto adr = i.key;

			if (auto pobject = RemoveConst(style).QueryProperty(adr))
			{
				auto [fn, idx] = i.value;

				fn(*self, *pobject, idx, stylesheet_flags);
			}
		}
	}

	Compile(*self);

	st_current_style = {};

	return self;
}

Reflex::TRef <Reflex::GLX::Detail::ComputedStyle> Reflex::GLX::Detail::ComputedStyle::ComputedStyle::Create(Size min, Size max)
{
	auto self = REFLEX_CREATE(ComputedStyleImpl);

	self->m_minmax = { min, max };
	
	SetFlag(self->m_propertyflags, ComputedStyle::kPropertyFlagMin, Reinterpret<UInt64>(min) != UInt64(0));

	SetFlag(self->m_propertyflags, ComputedStyle::kPropertyFlagMax, Reinterpret<UInt64>(max) != Reinterpret<UInt64>(kLarge));

	return self;
}

Reflex::TRef <Reflex::GLX::Detail::ComputedStyle> Reflex::GLX::Detail::ComputedStyle::Create(bool clipx, bool clipy)
{
	auto self = REFLEX_CREATE(ComputedStyleImpl);

	SetClip(*self, clipx, clipy);

	return self;
}

Reflex::TRef <Reflex::GLX::Detail::ComputedStyle> Reflex::GLX::Detail::ComputedStyle::Create(Float scale, Float opacity, Render render)
{
	auto self = REFLEX_CREATE(ComputedStyleImpl);

	SetScaleProperty(*self, scale);

	SetFlag(self->m_propertyflags, ComputedStyle::kPropertyFlagOpacity, opacity != 1.0f);

	self->m_opacity = opacity;

	SetRender(*self, render);

	return self;
}

Reflex::TRef <Reflex::GLX::Core::Renderer> Reflex::GLX::Detail::ComputedStyleImpl::CreateRenderer(GLX::Object & object) const
{
#if REFLEX_DEBUG
	auto flags = m_renderflags;
	SetFlag(flags, ComputedStyleImpl::kRenderFlagVisible, True(m_opacity));
	SetFlag(flags, ComputedStyleImpl::kRenderFlagClipX, m_clip.a);
	SetFlag(flags, ComputedStyleImpl::kRenderFlagClipY, m_clip.b);
	REFLEX_ASSERT(flags == m_renderflags);
#endif

	return CreateRendererBinder::Bind(m_renderflags & kCreateRendererFlags)(*this, object);
}

Reflex::TRef <Reflex::GLX::Detail::ComputedStyle> Reflex::GLX::Detail::ComputedStyleImpl::Mutate(const ComputedStyle & base) const
{
	auto clone = REFLEX_CREATE(ComputedStyleImpl);

	Copy(base, *clone);

	ApplyModifyByBinder::Bind(UInt8(m_propertyflags))(*clone, *this);

	return clone;
}

REFLEX_INLINE Reflex::GLX::Detail::ComputedStyleTransition::ComputedStyleTransition(ConstTRef <ComputedStyle> from, ConstTRef <ComputedStyle> to, ConstTRef <StateTransition> transition)
	: from(Cast<ComputedStyleImpl>(from))
	, to(Cast<ComputedStyleImpl>(to))
	, transition(transition)
{
	Retain(from);

	Retain(to);

	Retain(transition);
}

Reflex::TRef <Reflex::GLX::Detail::ComputedStyleTransition> Reflex::GLX::Detail::ComputedStyleTransition::Create(ConstTRef <ComputedStyle> from, ConstTRef <ComputedStyle> to, ConstTRef <StateTransition> transition)
{
	return New<CStyleTransitionImpl>(from, to, transition);
}

void Reflex::GLX::Detail::InitialiseCStyleSetters(Map < Address, Pair <decltype(&ComputedStyleImpl::NullPropertySetter),UInt16> > & setters)
{
	auto colour_t = GetTypeID<ColourProperty>();
	auto key32_t = GetTypeID<Data::Key32Property>();
	auto floats_t = GetTypeID<Data::ArrayOfFloat32Property>();
	auto float_t = GetTypeID<Data::Float32Property>();
	auto size_t = GetTypeID<SizeProperty>();
	auto margin_t = GetTypeID<MarginProperty>();
	auto string_t = GetTypeID<Data::CStringProperty>();
	auto layerdescs_t = GetTypeID<ArrayOfLayerDesc>();

	setters.Set(MakeAddress<Data::Int32Property>(K32("z_index")), { [](ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
	{
		auto & value = Cast<Data::Int32Property>(object)->value;	//vm TEMP -> should be Int8

		SetZIndex(self, Int8(value));
	} });

	setters.Set({ K32("z_index"), floats_t }, { [](ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
	{
		auto & value = Cast<Data::ArrayOfFloat32Property>(object)->value;

		SetZIndex(self, Int8(Truncate(value.GetFirst())));
	} });

	setters.Set({ kclip, key32_t }, { [](ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
	{
		switch (Cast<Data::Key32Property>(object)->value.value)
		{
		case ktrue:
		case K32("xy"):
			SetClip(self, true, true);
			break;

		case K32("x"):
			SetClip(self, true, false);
			break;

		case K32("y"):
			SetClip(self, false, true);
			break;

		default:
			SetClip(self, false, false);
			break;
		}
	} });



	//margins

	constexpr UInt32 kMarginKeys[] = { K32("margin"), K32("padding"), K32("render_pad") };

	REFLEX_LOOP_TYPE(UInt16, idx, 3)
	{
		auto id = kMarginKeys[idx];

		setters.Set({ id, margin_t }, { [](ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
		{
			SetMarginProperty(self, idx, Cast<MarginProperty>(object)->value);
		}, 
		idx});

		setters.Set({ id, floats_t }, { [](ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
		{
			SetMarginProperty(self, idx, ToMargin(Cast<Data::ArrayOfFloat32Property>(object)->value, stylesheet_flags));
		},
		idx });

		setters.Set({ id, string_t }, { [](ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
		{
			auto scale = GetStylesheetScale(st_current_style);

			auto string = ToView(Cast<Data::CStringProperty>(object)->value);

			auto lines = SplitCommaDelimited(string);

			Margin margin;

			switch (lines.GetSize())
			{
			case 4:
				margin = MakeMargin(ToFloat(scale, lines[0]), ToFloat(scale, lines[1]), ToFloat(scale, lines[2]), ToFloat(scale, lines[3]));
				break;

			case 2:
				margin = MakeMargin(ToFloat(scale, lines[0]), ToFloat(scale, lines[1]));
				break;

			case 1:
				margin = MakeMargin(ToFloat(scale, lines[0]));
				break;
			}

			SetMarginProperty(self, idx, margin);
		},
		idx });
	}


	
	//sizes

	constexpr UInt32 kSizeKeys[] = { ksize, K32("max") };

	REFLEX_LOOP_TYPE(UInt16, idx, 2)
	{
		auto id = kSizeKeys[idx];

		setters.Set({ id, size_t }, { [](ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
		{
			SetSizeProperty(self, idx, Cast<SizeProperty>(object)->value);
		},
		idx });

		setters.Set({ id, floats_t }, { [](ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
		{
			SetSizeProperty(self, idx, ToSize(Cast<Data::ArrayOfFloat32Property>(object)->value));
		},
		idx });

		setters.Set({ id, string_t }, { [](ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
		{
			SetSizeProperty(self, idx, ParseSizeScaled(GetStylesheetScale(st_current_style), Cast<Data::CStringProperty>(object)->value));
		},
		idx });
	}

	//setters.Set({ ksize, size_t }, { SizeSetter<&Detail::SetMin>::FromSize });
	//setters.Set({ ksize, floats_t }, { SizeSetter<&Detail::SetMin>::FromFloats });
	//setters.Set({ ksize, string_t }, { SizeSetter<&Detail::SetMin>::FromString });

	//setters.Set({ kmax, size_t }, { SizeSetter<&Detail::SetMax>::FromSize });
	//setters.Set({ kmax, floats_t }, { SizeSetter<&Detail::SetMax>::FromFloats });
	//setters.Set({ kmax, string_t }, { SizeSetter<&Detail::SetMax>::FromString });

	setters.Set({ K32("aspect_ratio"), float_t }, { FloatSetter<&Detail::SetAspectRatio>::FromFloat });
	setters.Set({ K32("aspect_ratio"), floats_t }, { FloatSetter<&Detail::SetAspectRatio>::FromFloats });

	setters.Set({ kscale, float_t }, { FloatSetter<&Detail::SetScaleProperty>::FromFloat });
	setters.Set({ kscale, floats_t }, { FloatSetter<&Detail::SetScaleProperty>::FromFloats });

	setters.Set({ kopacity, float_t }, { FloatSetter<&Detail::SetOpacityProperty>::FromFloat });
	setters.Set({ kopacity, floats_t }, { FloatSetter<&Detail::SetOpacityProperty>::FromFloats });

	setters.Set({ ktransition, float_t }, { FloatSetter<&Detail::SetTransition>::FromFloat });
	setters.Set({ ktransition, floats_t }, { FloatSetter<&Detail::SetTransition>::FromFloats });



	//bg_colour, colour

	static constexpr Pair <ComputedStyle::PropertyFlags, ComputedStyleImpl::RenderFlags> kColourFlags[2] =
	{
		{ ComputedStyle::kPropertyFlagBackgroundColor, ComputedStyleImpl::kRenderFlagBackgroundColour },
		{ ComputedStyle::kPropertyFlagNone, ComputedStyleImpl::kRenderFlagForegroundLayers }
	};

	REFLEX_LOOP_TYPE(UInt16, idx, 2)
	{
		auto keys = idx ? ToView(kColourPropertyKeys) : ToView(kBgColourPropertyKeys);
		
		for (auto id : keys)
		{
			setters.Set({ id, colour_t }, { [](ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
			{
				auto flags = kColourFlags[idx];
				
				self.m_colours[idx] = Cast<ColourProperty>(object)->value;

				SetFlag(self.m_propertyflags, flags.a);
				SetFlag(self.m_renderflags, flags.b);
			},
			idx });

			setters.Set({ id, floats_t }, { [](ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
			{
				auto flags = kColourFlags[idx];

				self.m_colours[idx] = ToColour(Cast<Data::ArrayOfFloat32Property>(object)->value);

				SetFlag(self.m_propertyflags, flags.a);
				SetFlag(self.m_renderflags, flags.b);
			}, 
			idx });

			setters.Set({ id, key32_t }, { [](ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
			{
				auto flags = kColourFlags[idx];

				auto id = Cast<Data::Key32Property>(object)->value;

				self.m_colours[idx] = FindResource<ColourProperty>(st_current_style, id)->value;

				SetFlag(self.m_propertyflags, flags.a);
				SetFlag(self.m_renderflags, flags.b);
			},
			idx });
		}
	}



	//render

	setters.Set({ krender, key32_t }, { [](ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
	{
		SetFlag(self.m_propertyflags, ComputedStyle::kPropertyFlagClipRenderDensity);

		auto attribute = Cast<Data::Key32Property>(object)->value.value;

		auto mode = ComputedStyle::kRenderAuto;

		self.m_render = mode;

		for (auto & i : kRenderModes)
		{
			if (i.b == attribute)
			{
				self.m_render = mode;

				break;
			}

			Reinterpret<UInt8>(mode)++;
		}

		self.m_render = mode;

		self.m_compile_renderflags = true;
	} });



	//bg & fg

	static constexpr Pair <ComputedStyle::PropertyFlags, ComputedStyleImpl::RenderFlags> kLayerFlags[2] =
	{
		{ ComputedStyle::kPropertyFlagBackgroundLayers, ComputedStyleImpl::kRenderFlagBackgroundLayers },
		{ ComputedStyle::kPropertyFlagForegroundLayers, ComputedStyleImpl::kRenderFlagForegroundLayers }
	};

	constexpr UInt32 kLayerKeys[] = { K32("bg"), K32("fg") };

	REFLEX_LOOP_TYPE(UInt16, idx, 2)
	{
		auto id = kLayerKeys[idx];

		setters.Set({ id, layerdescs_t }, { [](ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
		{
			CreateLayers(st_current_style, Cast<ArrayOfLayerDesc>(object), self.m_layers[idx]);

			auto flags = kLayerFlags[idx];
			SetFlag(self.m_propertyflags, flags.a);
			SetFlag(self.m_renderflags, flags.b);

			self.m_layersource[idx] = ToUIntNative(&object);
		}, 
		idx });

		setters.Set({ id, key32_t }, { [](ComputedStyleImpl & self, const Reflex::Object & object, UInt16 idx, UInt16 stylesheet_flags)
		{
			FindAndSetLayer(self.m_layers[idx], Cast<Data::Key32Property>(object));

			auto flags = kLayerFlags[idx];
			SetFlag(self.m_propertyflags, flags.a);
			SetFlag(self.m_renderflags, flags.b);

			self.m_layersource[idx] = ToUIntNative(&object);
		},
		idx });
	}
}

void Reflex::GLX::Detail::CreateLayers(const Style & style, const ArrayOfLayerDesc & property, ArrayOfLayer & layers)
{
	layers.Allocate(layers.GetSize() + property.value.GetSize());

	for (auto & i : property.value)
	{
		auto layer = Detail::Layer::Create(i->cls, style, *i);

		layers.Push<kAllocateNone>(layer);
	}
}

void Reflex::GLX::Detail::SetClip(ComputedStyleImpl & self, bool x, bool y)
{
	SetFlag(self.m_propertyflags, ComputedStyle::kPropertyFlagClipRenderDensity);

	self.m_clip = { x, y };

	self.m_compile_renderflags = true;
}

void Reflex::GLX::Detail::SetRender(ComputedStyleImpl & self, ComputedStyle::Render render, const Margin & pad)
{
	self.m_render = render;

	self.m_margins[2] = pad;

	SetFlag(self.m_propertyflags, ComputedStyle::kPropertyFlagClipRenderDensity);

	self.m_compile_renderflags = true;
}

void Reflex::GLX::Detail::SetReference(Data::PropertySet & properties, Key32 property_id, Key32 binding_id)
{
	properties.SetProperty(property_id, New<ReferenceProperty>(binding_id));
}

#pragma once

#include "reflex/glx/detail/defines.h"
#include "reflex/glx/detail/properties.h"
#include "reflex/glx/detail/layer.h"




//
//GenericLayer

REFLEX_NS(Reflex::GLX::Detail)

struct GenericLayer : public LayerImpl <GenericLayer>
{
	struct ObjectState;

	struct VTable
	{
		FunctionPointer <void(const GenericLayer & layer, ObjectState & data, GLX::Object & object)> OnBind = nullptr;
		FunctionPointer <void(const GenericLayer & layer, ObjectState & data, Size & contentsize)> OnAccommodate = nullptr;
		FunctionPointer <void(const GenericLayer & layer, ObjectState & data, Size size, Float & height)> OnAlign = nullptr;
		FunctionPointer <void(const GenericLayer & layer, const ObjectState & data, Size pixelsize, Colour & colour, Points & points)> OnVectoriseMonochrome = nullptr;
		FunctionPointer <void(const GenericLayer & layer, const ObjectState & data, Size pixelsize, ColourPoints & points)> OnVectoriseColour = nullptr;
		FunctionPointer <TRef<System::Renderer::Graphic>(const GenericLayer & self, ObjectState & data, Size pixelsize, UInt8 flags)> OnRedraw = nullptr;
	};

	template <class IMPL> static inline TRef <Reflex::Object> CreateSchema(Key32 uid);


	template <class IMPL> static inline TRef <Layer> Create(const Style & style, const Reflex::Object & schema_, const Data::PropertySet & params);



	//public (impls need access)

	void Vectorise(const ObjectState & data, const Size & pixelsize, Colour & colour, Points & points) const;

	void Vectorise(const ObjectState & data, const Size & pixelsize, ColourPoints & points) const;

	
	const ConstTRef <GenericPropertiesSchema> schema;

	const UInt16 stylesheet_flags;

	const UInt16 propertyflags;
	
	VTable vtable;
	



protected:

	friend Reflex::Detail::Constructor <GenericLayer>;

	GenericLayer(const Style & style, const Reflex::Object & schema_);

	GenericLayer(const Style & style, const Reflex::Object & schema_, const Data::PropertySet & params);
	
	~GenericLayer();


	template <bool COMPLEX, bool BINDINGS, bool CALLBACK> static TRef <Reflex::Object> CreateState(const Layer & self, GLX::Object & object);

	REFLEX_TBINDER_3P(CreateState);

	static void AccommodateDynamic(const GenericLayer & self, ObjectState & state, Size & size);

	static void AlignDynamic(const GenericLayer & self, ObjectState & state, Size size, Float & height);

	void RebindProperties(UInt level, ObjectState & data) const;

	void PrepareBindingLevel(UInt level, UInt8 stage, UInt8 stage_end);


	Array < Pair <Address,const GenericPropertiesSchema::Item*> > m_bindings;
	
	decltype(m_bindings)::View m_binding_stages[4];

	UInt8 m_static_state alignas(16)[4];
};

struct GenericLayer::ObjectState : public Reflex::Object
{
	ObjectState(GLX::Object & object, UInt16 propertyflags) : owner(object), propertyflags(propertyflags), reserved(0)
	{
		SetOnHeap(g_default_allocator);
	}

	template <class PROPERTIES> ConstTRef <PROPERTIES> GetProperties() const { return Reinterpret<PROPERTIES>(state); }

	template <class SCRATCH> TRef <SCRATCH> GetScratch(const GenericLayer & layer);

	template <class PROPERTIES, class SCRATCH> Pair < ConstTRef <PROPERTIES>, TRef <SCRATCH> > GetPropertiesAndScratch() { auto t = Reinterpret<Pair<PROPERTIES,SCRATCH>>(state); return { t->a, t->b }; }

	template <class PROPERTIES, class SCRATCH> Pair < ConstTRef <PROPERTIES>, ConstTRef <SCRATCH> > GetPropertiesAndScratch() const { auto t = Reinterpret<Pair<PROPERTIES,SCRATCH>>(state); return { t->a, t->b }; }


	const TRef <GLX::Object> owner;

	const UInt16 propertyflags;


	Reference <System::Renderer::Graphic> graphic;	//private/temp


	void * const reserved;

	UInt8 state alignas(16)[4];
};

struct GraphicRendererWithOffset : public Graphic
{
	GraphicRendererWithOffset(const Graphic & graphic, const Point & offset)
		: m_graphic(graphic),
		m_offset(offset)
	{
	}

	virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
	{
		m_graphic->Render(TransformMatrix(transform, m_offset), colour);
	}

	ConstReference <Graphic> m_graphic;

	Point m_offset;
};

struct GraphicRendererWithOffsetAndColour : public GraphicRendererWithOffset
{
	GraphicRendererWithOffsetAndColour(const Graphic & graphic, const Point & offset, const Colour & colour)
		: GraphicRendererWithOffset(graphic, offset),
		m_colour(colour)
	{
	}

	virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
	{
		GraphicRendererWithOffset::Render(transform, colour * m_colour);
	}

	Colour m_colour;
};

TRef <ArrayOfLayerDesc> ConvertLegacyLayers(const Data::ArrayOfCStringProperty & strings);

Alignment OrientationToAlignment(Pair <Orientation> xy);

REFLEX_END




//
//impl

template <class IMPL> inline Reflex::TRef <Reflex::Object> Reflex::GLX::Detail::GenericLayer::CreateSchema(Key32 uid)
{
	typedef typename IMPL::Properties Properties;

	typedef typename IMPL::Scratch Scratch;

	typedef Pair <Properties,Scratch> State;

	auto schema = New<GenericPropertiesSchema>();

	schema->uid = uid;

	schema->state_size = sizeof(State);

	schema->scratch_offset = REFLEX_OFFSETOF(State, b);

	schema->complex = (!(IsRawCopyable<Properties>::value && IsRawCopyable<Scratch>::value));

	schema->state_ctr_dtr =
	{
		[](void * adr, const void * value)
		{
			Reflex::Detail::Constructor<State>::Construct(adr, *Cast<State>(value));
		},
		[](void * adr)
		{
			Reflex::Detail::Constructor<State>::Destruct(Cast<State>(adr));
		},
	};

	REFLEX_STATIC_ASSERT(sizeof(State) <= sizeof(GenericPropertiesSchema::default_state));

	Reflex::Detail::Constructor<State>::Construct(schema->default_state);

	IMPL::Init(schema);	//pass scratch here to optional initalise if needed

	schema->FinaliseBindings();

	return schema;
}

template <class IMPL> inline Reflex::TRef <Reflex::GLX::Detail::Layer> Reflex::GLX::Detail::GenericLayer::Create(const Style & style, const Reflex::Object & schema_, const Data::PropertySet & params)
{
	typedef typename IMPL::Properties Properties;

	typedef typename IMPL::Scratch Scratch;

	typedef Pair <Properties,Scratch> State;

	constexpr auto kSize = sizeof(GenericLayer) + sizeof(State);

	auto heap = Reflex::g_default_allocator->Allocate(kSize, {});

	return Reflex::Detail::Constructor<GenericLayer>::Construct(heap, style, schema_, params);
}

inline void Reflex::GLX::Detail::GenericLayer::Vectorise(const ObjectState & data, const Size & pixelsize, Colour & colour, Points & points) const
{
	vtable.OnVectoriseMonochrome(*this, data, pixelsize, colour, points);
}

inline void Reflex::GLX::Detail::GenericLayer::Vectorise(const ObjectState & data, const Size & pixelsize, ColourPoints & output) const
{
	vtable.OnVectoriseColour(*this, data, pixelsize, output);
}

template <class SCRATCH> Reflex::TRef <SCRATCH> Reflex::GLX::Detail::GenericLayer::ObjectState::GetScratch(const GenericLayer & self)
{ 
	return Reinterpret<SCRATCH>(state + self.schema->scratch_offset);
}

#pragma once

#include "countable.h"




//
//declarations

REFLEX_NS(Reflex::GLX::Detail)

class Layer;

template <class IMPL> class LayerImpl;

struct LayerState : public Reflex::Object
{
	LayerState(TRef <GLX::Object> object)
		: object(object)
	{
	}

	const TRef <GLX::Object> object;
};

REFLEX_END



//
//layer

REFLEX_SET_TRAIT(Reflex::GLX::Detail::Layer, IsSingleThreadExclusive)

class Reflex::GLX::Detail::Layer :
	public Reflex::Object,
	public Countable <MakeKey32("Layer")>
{
public:

	static Layer & null;



	//types

	class Class;

	using Factory = Sequence <Key32,const Class*>;


	using InitSchema = FunctionPointer <TRef<Reflex::Object>(Key32 uid)>;

	using Ctr = FunctionPointer <TRef<Layer>(const Style &, const Reflex::Object & schema, const Data::PropertySet & properties)>;


	enum OptimisationFlags : UInt8
	{
		kOptimisationFlagNotResponsive = 1,
		kOptimisationFlagVector = 2,
	};

	

	//global

	[[nodiscard]] static TRef <Layer> Create(const Class & cls, const Style & style, const Data::PropertySet & params);



	//data

	[[nodiscard]] REFLEX_INLINE TRef <Reflex::Object> CreateState(GLX::Object & object) const 
	{
		return OnCreateState(*this, object);
	}

	REFLEX_INLINE void Accommodate(Reflex::Object & object_data, Size & contentsize) const
	{
		OnAccommodate(*this, object_data, contentsize);
	}

	REFLEX_INLINE void Align(Reflex::Object & object_data, Size size, Float & contenth) const
	{
		OnAlign(*this, object_data, size, contenth);
	}

	REFLEX_INLINE TRef <System::Renderer::Graphic> Redraw(Object & object_data, Size pixelsize) const
	{
		return OnRedraw(*this, object_data, pixelsize, 0);
	}



	//info

	const UInt8 flags;



protected:

	Layer(UInt8 flags = 0) : flags(flags) {}
	
	
	FunctionPointer <TRef<Object>(const Layer & self, GLX::Object & object)> OnCreateState = [](const Layer & self, GLX::Object & owner) { return Null<Reflex::Object>(); };

	FunctionPointer <void(const Layer & self, Object & object_data, Size & contentsize)> OnAccommodate = [](const Layer & self, Object & object_data, Size & contentsize) {};

	FunctionPointer <void(const Layer & self, Object & object_data, Size size, Float & contentheight)> OnAlign = [](const Layer & self, Object & object_data, Size size, Float & contentheight) {};

	FunctionPointer <TRef <System::Renderer::Graphic>(const Layer & self, Object & object_data, Size pixelsize, UInt8 flags)> OnRedraw = 0;

};




//
//typed

template <class IMPL>
class Reflex::GLX::Detail::LayerImpl : public Layer
{
protected:

	using Layer::Layer;


	void SetOnCreateState(FunctionPointer <TRef<Object>(const IMPL & self, GLX::Object & object)> callback);

	template <class STATE> void SetOnAccommodate(FunctionPointer <void(const IMPL & self, STATE & state, Size & contentsize)> callback);

	template <class STATE> void SetOnAlign(FunctionPointer <void(const IMPL & self, STATE & state, Size size, Float & contenth)> callback);

	template <class STATE> void SetOnRedraw(FunctionPointer <TRef<System::Renderer::Graphic>(const IMPL & self, STATE & state, Size pixelsize, UInt8 flags)> callback);

};



//
//class

class Reflex::GLX::Detail::Layer::Class : public Reflex::Detail::StaticItem <Class>
{
public:

	static Class & null;


	static const Class * Query(Key32 key, const Class * fallback = nullptr);


	Class(const CString::View & id, InitSchema initschema, Ctr ctr, bool enable = true)
		: id(id),
		initschema(initschema),
		ctr(ctr),
		enabled(enable)
	{
	}

	const CString::View id;

	const InitSchema initschema;

	const Ctr ctr;

	ConstTRef <Reflex::Object> schema;

	bool enabled;
};




//
//impl

Reflex::TRef <Reflex::GLX::Detail::Layer> inline Reflex::GLX::Detail::Layer::Create(const Class & cls, const Style & style, const Data::PropertySet & params)
{
	return cls.ctr(style, cls.schema, params);
}

template <class IMPL> inline void Reflex::GLX::Detail::LayerImpl<IMPL>::SetOnCreateState(FunctionPointer<TRef<Object>(const IMPL & self, GLX::Object & object)> callback)
{
	Layer::OnCreateState = reinterpret_cast<decltype(Layer::OnCreateState)>(callback);
}

template <class IMPL> template <class STATE> inline void Reflex::GLX::Detail::LayerImpl<IMPL>::SetOnAccommodate(FunctionPointer<void(const IMPL & self, STATE & state, Size & contentsize)> callback)
{
	Layer::OnAccommodate = reinterpret_cast<decltype(Layer::OnAccommodate)>(callback);
}

template <class IMPL> template <class STATE> inline void Reflex::GLX::Detail::LayerImpl<IMPL>::SetOnAlign(FunctionPointer<void(const IMPL & self, STATE & state, Size size, Float & contenth)> callback)
{
	Layer::OnAlign = reinterpret_cast<decltype(Layer::OnAlign)>(callback);
}

template <class IMPL> template <class STATE> inline void Reflex::GLX::Detail::LayerImpl<IMPL>::SetOnRedraw(FunctionPointer<TRef<System::Renderer::Graphic>(const IMPL & self, STATE & state, Size pixelsize, UInt8 flags)> callback)
{
	Layer::OnRedraw = reinterpret_cast<decltype(Layer::OnRedraw)>(callback);
}

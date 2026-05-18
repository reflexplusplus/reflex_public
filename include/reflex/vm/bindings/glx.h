#pragma once

#include "../../glx.h"




REFLEX_NS(Reflex::GLXVM)

template <class TYPE> struct WidgetOf;

struct Object;

struct Type;

struct DragHandler;


template <class TYPE> inline TRef <GLX::Object> CreateWidgetWithContext(VM::Context & context);

template <class TYPE> inline TRef <GLX::Object> CreateWidget(VM::Context & context);


void AttachCircular(Data::PropertySet & object, VM::Context & context);	//do this before adding any non-WidgetOf to context


extern const VM::Module gGeometry, gBitmap, gGLX, gGraphic;

REFLEX_END




//
//

template <class TYPE>
struct Reflex::GLXVM::WidgetOf :
	public TYPE,
	public VM::Detail::Circular
{
	template <class ...VARGS> WidgetOf(VM::Context & context, VARGS &&... v)
		: TYPE(std::forward<VARGS>(v)...),
		VM::Detail::Circular(context, *this)
	{
	}

	virtual void OnSetProperty(Address address, Reflex::Object & property) override
	{
		if (address.type_id == GetTypeID<Data::UInt8Property>())
		{
			bool value = Cast<Data::UInt8Property>(property)->value;

			TYPE::OnSetProperty(MakeAddress<Data::BoolProperty>(address.id), New<Data::BoolProperty>(value));
		}
		else if (address.type_id == GetTypeID<ObjectOf<Pair<UInt8>>>())
		{
			Pair <UInt8> value = Cast<ObjectOf<Pair<UInt8>>>(property)->value;

			TYPE::OnSetProperty(MakeAddress<ObjectOf<Pair<bool>>>(address.id), New<ObjectOf<Pair<bool>>>(MakeTuple(True(value.a), True(value.b))));
		}

		TYPE::OnSetProperty(address, property);
	}
};

struct Reflex::GLXVM::Object : WidgetOf <GLX::Object>
{
	REFLEX_OBJECT_EX(GLXVM::Object, GLX::Object, "GLX::Object");

	REFLEX_DECLARE_KEY32(Extension);

	struct Callbacks;

	using Methods = ObjectOf < Sequence < Key32, Reference <VM::Detail::FnObject,kReferenceOnHeap> > >;


	Object(VM::Context & context);

	void OnClock(Float32 delta) override;

	void OnSetProperty(Address address, Reflex::Object & object) override;

	void OnUnsetProperty(Address address) override;

	void OnReleaseData() override;

	using Reflex::Object::GetActualRetainCount;


	const TRef <VM::Context> context;

	const Callbacks * m_callbacks;
};

struct Reflex::GLXVM::DragHandler : public Reflex::Detail::StaticItem <DragHandler>
{
	DragHandler(TypeID vmtype, TypeID apptype, const Function <Reflex::Object&(Reflex::Object&)> & convert, const Function <void(VM::Context&,Key32,Reflex::Object&,Data::PropertySet&)> & fn) : vmtype(vmtype), apptype(apptype), convert(convert), fn(fn) {}

	TypeID vmtype;

	TypeID apptype;

	Function <Reflex::Object&(Reflex::Object&)> convert;

	Function <void(VM::Context&,Key32,Reflex::Object&,Data::PropertySet&)> fn;
};




//
//impl

REFLEX_NS(Reflex::GLXVM)

template <class TYPE, bool CTR = std::is_default_constructible<TYPE>::value>
struct WidgetCreator
{
	static TRef <GLX::Object> Create(VM::Context & context)
	{
		return REFLEX_CREATE(WidgetOf<TYPE>, context);
	}
};

template <class TYPE>
struct WidgetCreator <TYPE,false>
{
	static TRef <GLX::Object> Create(VM::Context & context)
	{
		auto o = TYPE::Create();

		SetProperty(o, kNullKey, REFLEX_CREATE(ObjectOf<VM::Detail::Circular>, context, o));

		return o;
	}
};

REFLEX_END

template <class TYPE> inline Reflex::TRef <Reflex::GLX::Object> Reflex::GLXVM::CreateWidgetWithContext(VM::Context & context)
{
	return REFLEX_CREATE(TYPE, context);
}

template <class TYPE> inline Reflex::TRef <Reflex::GLX::Object> Reflex::GLXVM::CreateWidget(VM::Context & context)
{
	return WidgetCreator<TYPE>::Create(context);
}

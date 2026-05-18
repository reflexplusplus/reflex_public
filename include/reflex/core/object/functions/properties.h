#pragma once

#include "../objectof.h"
#include "../../function.h"
#include "new.h"
#include "null.h"




//
//Primary API

namespace Reflex
{

	using AbstractProperty = Object;

	void UnsetAbstractProperty(Object & owner, Key32 id);

	void SetAbstractProperty(Object & owner, Key32 id, TRef <AbstractProperty> property);

	AbstractProperty* QueryAbstractProperty(Object & owner, Key32 id, AbstractProperty * fallback = nullptr);

	TRef <AbstractProperty> GetAbstractProperty(Object & owner, Key32 id);

	ConstTRef <AbstractProperty> GetAbstractProperty(const Object & owner, Key32 id);


	template <class TYPE, class ... VARGS> Reference <TYPE> AcquireProperty(Object & object, Key32 id, VARGS &&... v);

	template <class TYPE, class const_auto_1> ConstTRef <TYPE> GetProperty(const_auto_1 && objectref, Key32 id, const TYPE & fallback = Null<TYPE>());	//returns property or null instance


	template <class SIG> void UnsetFunctionProperty(Object & owner, Key32 id);

	template <class FN_TYPE> void SetFunctionProperty(Object & owner, Key32 id, FN_TYPE && fn);

	template <class SIG> const auto * QueryFunctionProperty(const Object & owner, Key32 id);

}




//
//impl

REFLEX_NS(Reflex::Detail)

template <class TYPE>
struct ValueTypeUnwrapper
{
	using Type = TYPE;
};

template <class TYPE>
struct ValueTypeUnwrapper < ArrayRegion <TYPE> >
{
	using Type = Array < NonConstT <TYPE> >;
};

template <class SIG> using FunctionObject = ObjectOf <Function <SIG>>;

REFLEX_END

inline Reflex::TRef <Reflex::Object> Reflex::GetAbstractProperty(Object & owner, Key32 id)
{
	return QueryAbstractProperty(owner, id, &Detail::GetNullInstance<Object>());
}

inline Reflex::ConstTRef <Reflex::Object> Reflex::GetAbstractProperty(const Object & owner, Key32 id)
{
	return QueryAbstractProperty(RemoveConst(owner), id, &Detail::GetNullInstance<Object>());
}

template <class TYPE, class ... VARGS> inline Reflex::Reference <TYPE> Reflex::AcquireProperty(Object & object, Key32 id, VARGS &&... v)
{
	REFLEX_STATIC_ASSERT_OBJECT_TYPE(TYPE);

	auto address = MakeAddress<TYPE>(id);

	if (auto fallback = object.QueryProperty(address))
	{
		return Cast<TYPE>(fallback);
	}
	else
	{
		auto temp = Make<TYPE>(std::forward<VARGS>(v)...);

		object.SetProperty(address, temp);

		return std::move(temp);
	}
}

template <class TYPE, class const_auto_1> inline Reflex::ConstTRef <TYPE> Reflex::GetProperty(const_auto_1 && objectref, Key32 id, const TYPE & fallback)
{
	REFLEX_STATIC_ASSERT_OBJECT_TYPE(TYPE);

	auto & object = Deref(objectref);

	Object * ptr = RemoveConst(object).QueryProperty(MakeAddress<TYPE>(id), RemoveConst(&fallback));

	return Cast<TYPE>(ptr);
}

template <class auto_1> inline void Reflex::SetFunctionProperty(Object & owner, Key32 id, auto_1 && fn)
{
	using Signature = typename Detail::FunctionTraits < NonConstT <NonRefT <auto_1> > >::Signature;

	owner.SetProperty(id, New<Detail::FunctionObject<Signature>>(std::forward<auto_1>(fn)));
}

template <class SIG> inline void Reflex::UnsetFunctionProperty(Object & owner, Key32 id)
{
	owner.UnsetProperty<Detail::FunctionObject<SIG>>(id);
}

template <class SIG> inline const auto * Reflex::QueryFunctionProperty(const Object & owner, Key32 id)
{
	return owner.QueryProperty<Detail::FunctionObject<SIG>>(id);
}

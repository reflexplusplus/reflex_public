#pragma once

#include "detail/iterator.h"




//
//Primary API

namespace Reflex::Data
{

	template <class TYPE> void UnsetAll(PropertySet & object);


	void Assimilate(Data::PropertySet & target, const Data::PropertySet & src);

	Data::PropertySet Merge(const Data::PropertySet & a, const Data::PropertySet & b);

}




//
//impl

REFLEX_NS(Reflex::Data::Detail)

template <class TYPE, class ... VARGS> inline TRef <TYPE> AcquireProperty(PropertySet & dynamic, Key32 id, VARGS &&... v)
{
	REFLEX_STATIC_ASSERT_OBJECT_TYPE(TYPE);

	auto address = MakeAddress<TYPE>(id);

	if (auto fallback = dynamic.QueryProperty(address))
	{
		return Cast<TYPE>(fallback);
	}
	else
	{
		auto temp = Reflex::New<TYPE>(std::forward<VARGS>(v)...);

		dynamic.SetProperty(address, temp);

		return temp;
	}
}

template <class REFERENCE_T> inline REFERENCE_T SetObjectImpl(Object & object, Key32 id, typename REFERENCE_T::Type & p)
{
	REFERENCE_T temp = p;

	object.SetProperty(MakeAddress<typename REFERENCE_T::Type>(id), p);

	return std::move(temp);
}

template <class REFERENCE_T, class PROPERTY> inline REFERENCE_T SetValueImpl(Object & object, Key32 id, const PROPERTY & p)
{
	REFERENCE_T temp = REFLEX_CREATE(ObjectOf<PROPERTY>, p);

	object.SetProperty(MakeAddress<typename REFERENCE_T::Type>(id), temp);

	return std::move(temp);
}

template <class auto_1, class auto_2> inline auto SetPropertyEx(auto_1 && objectref, Key32 id, auto_2 && pref)
{
	auto & object = Deref(objectref);

	auto & p = Deref(pref);

	using ObjectOrValueType = typename Reflex::Detail::ValueTypeUnwrapper < NonConstT < NonRefT < decltype(p) > > >::Type;

	using ReferenceType = TRef < ObjectType <ObjectOrValueType> >;

	if constexpr (IsObject<ObjectOrValueType>::value)
	{
		Detail::SetObjectImpl<ReferenceType>(object, id, p);
	}
	else
	{
		Detail::SetValueImpl<ReferenceType, ObjectOrValueType>(object, id, p);
	}
}

constexpr UInt64 MakeKey64(UInt32 high, UInt32 low)
{
	return (static_cast<UInt64>(high) << 32) | static_cast<UInt64>(low);
}

template <class FIRST> inline void MakeDynamicR(PropertySet & dynamic, Key32 id, FIRST && first)
{
	Detail::SetPropertyEx(dynamic, id, std::forward<FIRST>(first));
}

template <class FIRST, class... REST> inline void MakeDynamicR(PropertySet & dynamic, Key32 id, FIRST && first, REST && ... rest)
{
	Detail::SetPropertyEx(dynamic, id, std::forward<FIRST>(first));

	MakeDynamicR(dynamic, std::forward<REST>(rest)...);
}

template <class ... VARGS> PropertySet MakePropertySet(VARGS && ... vargs);

REFLEX_END

template <class ... VARGS> inline Reflex::Data::PropertySet Reflex::Data::Detail::MakePropertySet(VARGS && ... vargs)
{
	PropertySet dynamic;

	MakeDynamicR(dynamic, std::forward<VARGS>(vargs)...);

	return dynamic;
}

template <class TYPE> inline void Reflex::Data::UnsetAll(PropertySet & dynamic)
{
	dynamic.UnsetAll(GetTypeID<TYPE>());
}

inline Reflex::Data::PropertySet Reflex::Data::Merge(const Data::PropertySet & a, const Data::PropertySet & b)
{
	PropertySet sum = a;

	Assimilate(sum, b);

	return sum;
}

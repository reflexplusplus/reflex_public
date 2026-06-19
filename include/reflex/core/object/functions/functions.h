#pragma once

#include "../reference.h"




//
//Primary API

namespace Reflex
{

	bool operator==(const Object & a, const Object & b);

	bool operator!=(const Object & a, const Object & b);


	template <class POLYTYPE, class TYPE> decltype(auto) Cast(TYPE & type);

	template <class POLYTYPE, class TYPE> decltype(auto) Cast(const TYPE & type);


	template <class TYPE> TYPE * DynamicCast(Object & object, TYPE * fallback = nullptr);

	template <class TYPE> const TYPE * DynamicCast(const Object & object, const TYPE * fallback = nullptr);


	template <class auto_t> bool IsNull(auto_t && object);

	template <class auto_t> bool IsValid(auto_t && object);


	template <class auto_t> auto AutoRelease(auto_t && object);


	template <class TYPE> TYPE & Deref(TRef <TYPE> type) { return *type; }

	template <class TYPE> TYPE & Deref(CommonReference <TYPE> & ref) { return *ref; }
	
	template <class TYPE> TYPE & Deref(const CommonReference <TYPE> & ref) { return *ref; }

	template <class TYPE, ReferenceSafeFlags SAFETY> TYPE & Deref(Reference <TYPE, SAFETY> & ref) { return *ref; }

	template <class TYPE, ReferenceSafeFlags SAFETY> TYPE & Deref(const Reference <TYPE, SAFETY> & ref) { return *ref; }


	template <class TYPE> TRef <TYPE> * GetAdr(TRef <TYPE> & tref) { return Reinterpret<TRef<TYPE>>(&Reinterpret<TYPE*>(tref)); }

	template <class TYPE> const TRef <TYPE> * GetAdr(const TRef <TYPE> & tref) { return Reinterpret<TRef<TYPE>>(&Reinterpret<TYPE*>(tref)); }

}




//
//impl

#define REFLEX_STATIC_ASSERT_DYNAMIC_CASTABLE(TYPE) REFLEX_STATIC_ASSERT(Reflex::kIsType<typename TYPE::DynamicCastableType, TYPE>)

REFLEX_NS(Reflex::Detail)

bool IsOrInheritsFrom(DynamicTypeRef rttypeinfo, DynamicTypeRef object_t);

bool CheckObjectType(const Object & object, DynamicTypeRef object_t);

template <class POLYTYPE, class TYPE> REFLEX_INLINE POLYTYPE & ApplyCast(TYPE & type)
{
	if constexpr (IsType<NonConstT<POLYTYPE>, NonConstT<TYPE>>::value)
	{
		REFLEX_STATIC_ASSERT("unnecesary Cast");
	}

	return static_cast<POLYTYPE&>(type);
}

REFLEX_END

REFLEX_INLINE bool Reflex::operator==(const Object & a, const Object & b)
{
	return &a == &b;
}

REFLEX_INLINE bool Reflex::operator!=(const Object & a, const Object & b)
{
	return &a != &b;
}

template <class TYPE> REFLEX_INLINE TYPE * Reflex::DynamicCast(Object & object, TYPE * fallback)
{
	REFLEX_STATIC_ASSERT_OBJECT_TYPE(TYPE);
	REFLEX_STATIC_ASSERT_DYNAMIC_CASTABLE(TYPE);

	Detail::DynamicTypeRef target_t = TYPE::kDynamicTypeInfo;

	REFLEX_ASSERT(target_t != Object::kDynamicTypeInfo);

	auto itr = object.object_t;

	do
	{
		if (itr == target_t) return Cast<TYPE>(&object);
	}
	while ((itr = itr->base));

	return fallback;
}

template <class TYPE> REFLEX_INLINE const TYPE * Reflex::DynamicCast(const Object & object, const TYPE * fallback)
{
	return DynamicCast<TYPE>(RemoveConst(object), RemoveConst(fallback));
}

REFLEX_INLINE bool Reflex::Detail::IsOrInheritsFrom(DynamicTypeRef itr, DynamicTypeRef object_t)
{
	do
	{
		if (itr == object_t) return true;
	}
	while ((itr = itr->base));

	return false;
}

REFLEX_INLINE bool Reflex::Detail::CheckObjectType(const Object & object, DynamicTypeRef object_t)
{
	return IsOrInheritsFrom(object.object_t, object_t);
}

template <class POLYTYPE, class TYPE> inline decltype(auto) Reflex::Cast(TYPE & type)
{
	if constexpr (kIsReflexReference<TYPE>)
	{
		using ConstT = ConditionalType <kIsConst<decltype(*type)>, const POLYTYPE, POLYTYPE>;

		return TRef<ConstT>(static_cast<ConstT&>(RemoveConst(*type)));
	}
	else if constexpr (kIsObject<POLYTYPE>)
	{
		return TRef<POLYTYPE>(Detail::ApplyCast<POLYTYPE&>(type));
	}
	else
	{
		return Detail::ApplyCast<POLYTYPE>(type);
	}
}

template <class POLYTYPE, class TYPE> inline decltype(auto) Reflex::Cast(const TYPE & type)
{
	if constexpr (kIsReflexReference<TYPE>)
	{
		using ConstT = ConditionalType <kIsConst<decltype(*type)>, const POLYTYPE, POLYTYPE>;

		return TRef<ConstT>(static_cast<ConstT&>(RemoveConst(*type)));
	}
	else if constexpr (kIsObject<POLYTYPE>)
	{
		return ConstTRef<POLYTYPE>(Detail::ApplyCast<const POLYTYPE&>(type));
	}
	else
	{
		return Detail::ApplyCast<const POLYTYPE&>(type);
	}
}

template <class auto_t> REFLEX_INLINE auto Reflex::AutoRelease(auto_t && object)
{
	constexpr bool test = !IsReflexReference<NonRefT<auto_t>>::retaining;

	static_assert(test, "unnecesary AutoRelease");

	return Reference(Deref(object));
}

template <class auto_t> REFLEX_INLINE bool Reflex::IsNull(auto_t && objectref)
{
	auto & object = Deref(objectref);

	using Type = NonConstT< NonRefT <decltype(object)> >;

	REFLEX_STATIC_ASSERT(kIsObject<Type>);

	return &object == &Detail::GetNullInstance<Type>();
}

template <class auto_t> REFLEX_INLINE bool Reflex::IsValid(auto_t && objectref)
{
	auto & object = Deref(objectref);

	using Type = NonConstT< NonRefT <decltype(object)> >;

	REFLEX_STATIC_ASSERT(kIsObject<Type>);

	return &object != &Detail::GetNullInstance<Type>();
}

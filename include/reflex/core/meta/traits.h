#pragma once

#include "../types.h"
#include "nothing.h"




//
//Secondary API

#define REFLEX_SET_TRAIT_EX(TYPE,TRAIT,VALUE) namespace Reflex { template <> struct TRAIT <TYPE> { static constexpr bool value = VALUE; }; }

#define REFLEX_SET_TRAIT(TYPE,TRAIT) REFLEX_SET_TRAIT_EX(TYPE,TRAIT,true)

#define REFLEX_SET_TRAIT_TEMPLATED(TEMPLATE_TYPE,TRAIT) namespace Reflex { template <class TYPE> struct TRAIT < TEMPLATE_TYPE <TYPE> > { static constexpr bool value = true; }; }

#define REFLEX_PUBLISH_TRAIT_VALUE(TRAIT) template <class TYPE> constexpr auto k##TRAIT = TRAIT<TYPE>::value;

namespace Reflex
{


	template <class TYPE> struct IsRawConstructible;

	template <class TYPE> struct IsRawCopyable { static constexpr bool value = bool(__is_trivially_copyable(TYPE)); };

	template <class TYPE> struct IsRawComparable;


	template <class TYPE> struct IsClass { static constexpr bool value = bool(__is_class(TYPE)); };

	template <class TYPE> struct IsEnum { static constexpr bool value = bool(__is_enum(TYPE)); };

	template <class TYPE> struct IsScalar { static constexpr bool value = std::is_scalar_v <TYPE>; };


	template <class TYPE> struct IsConst { static constexpr bool value = false; };

	template <class TYPE> struct IsConst <const TYPE> { static constexpr bool value = true; };


	template <class TYPE> struct IsPointer { static constexpr bool value = false; };

	template <class TYPE> struct IsPointer <TYPE*> { static constexpr bool value = true; };


	template <class TYPE> struct IsReference { static constexpr bool value = false; };

	template <class TYPE> struct IsReference <TYPE&> { static constexpr bool value = true; };


	template <class A, class ... B> struct IsType;

	template <class TYPE, class BASE> using InheritsFrom = std::is_base_of <BASE, TYPE>;	//struct InheritsFrom;


	template <class TYPE> struct IsBoolCastable { static constexpr bool value = std::is_convertible_v <TYPE,bool>; };


	template <class TYPE> struct IsAbstract { static constexpr bool value = bool(__is_abstract(TYPE)); };


	template <class TYPE> struct SizeOf { static constexpr UInt value = sizeof(TYPE); };

	template <> struct SizeOf <void> { static constexpr UInt value = 0; };

	template <> struct SizeOf <NullType> { static constexpr UInt value = 0; };


	template <class TYPE> struct IsTrivial { static constexpr bool value = std::is_trivially_copyable<TYPE>::value; };

	template <class TYPE> struct IsObject { static constexpr bool value = InheritsFrom<TYPE,Object>::value || IsType<TYPE,Object>::value; };


	REFLEX_PUBLISH_TRAIT_VALUE(IsRawCopyable);
	REFLEX_PUBLISH_TRAIT_VALUE(IsClass);
	REFLEX_PUBLISH_TRAIT_VALUE(IsEnum);
	REFLEX_PUBLISH_TRAIT_VALUE(IsScalar);
	REFLEX_PUBLISH_TRAIT_VALUE(IsConst);
	REFLEX_PUBLISH_TRAIT_VALUE(IsPointer);
	REFLEX_PUBLISH_TRAIT_VALUE(IsReference);
	REFLEX_PUBLISH_TRAIT_VALUE(IsBoolCastable);
	REFLEX_PUBLISH_TRAIT_VALUE(IsAbstract);
	REFLEX_PUBLISH_TRAIT_VALUE(IsRawConstructible);
	REFLEX_PUBLISH_TRAIT_VALUE(IsRawComparable);
	REFLEX_PUBLISH_TRAIT_VALUE(SizeOf);
	REFLEX_PUBLISH_TRAIT_VALUE(IsTrivial);
	REFLEX_PUBLISH_TRAIT_VALUE(IsObject);

	template <class A, class B> static constexpr bool kIsType = IsType<A,B>::value;

}




//
//impl

#define REFLEX_STATIC_ASSERT_OBJECT_TYPE(TYPE) static_assert(kIsObject<TYPE>, "TYPE is not an Object");

#define REFLEX_ASSERT_RAW(TYPE) REFLEX_STATIC_ASSERT(std::is_trivially_copyable<TYPE>::value)

REFLEX_ASSERT_RAW(Reflex::NullType);

REFLEX_NS(Reflex::Detail)

template<class T, class B = T>
struct HasOperatorEqual
{
	template<class U, class V>
	static auto test(U*) -> decltype(std::declval<U>() == std::declval<V>());

	template<typename, typename>
	static auto test(...) -> std::false_type;

	static constexpr bool value = std::is_same<bool, decltype(test<T,B>(0))>::value;
};

template <class T, class B = T>
struct HasOperatorInequal
{
	template<class U, class V>
	static auto test(U*) -> decltype(std::declval<U>() != std::declval<V>());

	template<typename, typename>
	static auto test(...) -> std::false_type;

	static constexpr bool value = std::is_same<bool, decltype(test<T,B>(0))>::value;
};

template <class TYPE, bool RAWCOPY>
struct IsRawConstructibleWorkaround	//XCODE_WORKAROUND
{
	static constexpr bool value = false;
};

template <class TYPE>
struct IsRawConstructibleWorkaround <TYPE,true>	//XCODE_WORKAROUND
{
	static constexpr bool value = __is_trivially_constructible(TYPE);
};

template <class A, class B>
struct IsTypeImpl
{
	static constexpr bool value = false;
};

template <class A>
struct IsTypeImpl <A, A>
{
	static constexpr bool value = true;
};

REFLEX_END

REFLEX_SET_TRAIT(void,IsRawCopyable)

template <class TYPE>
struct Reflex::IsRawConstructible
{
	static constexpr bool value = Detail::IsRawConstructibleWorkaround<TYPE,std::is_trivially_copyable<TYPE>::value>::value;
};

template <class TYPE>
struct Reflex::IsRawComparable
{
	static constexpr bool value = IsRawCopyable<TYPE>::value && (IsScalar<TYPE>::value || !(Detail::HasOperatorEqual<TYPE>::value || Detail::HasOperatorInequal<TYPE>::value));
};

template <class A, class... REST>
struct Reflex::IsType
{
	static constexpr bool value = (Detail::IsTypeImpl<A,REST>::value || ...);
};

#pragma once

#include "traits.h"
#include "object.h"
#include "detail/nullable.h"
#include "../tuple.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE> class ObjectOf;

	template <class TYPE> using ObjectType = ConditionalType < kIsObject<TYPE>, TYPE, ObjectOf <TYPE> >;

}




//
//ObjectOf

template <class TYPE>
class Reflex::ObjectOf : public Object
{
public:

	REFLEX_STATIC_ASSERT(!kIsObject<TYPE>);

	REFLEX_OBJECT_EX(ObjectOf, Object, "{ObjectOf}");

	using ValueType = TYPE;


	template <class ... VARGS> ObjectOf(VARGS && ... args);

	ObjectOf(const ObjectOf &) = delete;

	ObjectOf & operator=(const ObjectOf &) = delete;


	TYPE value = {};
};




//
//impl

template <class A, class B> class Reflex::ObjectOf < Reflex::Tuple <A,B> > : public Object
{
public:

	REFLEX_OBJECT_EX(ObjectOf, Object, "{ObjectOf}");

	using ValueType = Tuple <A,B>;

	ObjectOf() = default;

	ObjectOf(const ValueType & value) : value(value) {}

	ObjectOf(ValueType & value) : value(std::move(value)) {}

	ObjectOf(const A & a, const B & b) : value({a,b}) {}

	ObjectOf(const ObjectOf &) = delete;

	ObjectOf & operator=(const ObjectOf &) = delete;

	ValueType value;
};

template <class A, class B, class C> class Reflex::ObjectOf < Reflex::Tuple <A,B,C> > : public Object
{
public:

	REFLEX_OBJECT_EX(ObjectOf, Object, "{ObjectOf}");

	using ValueType = Tuple <A,B,C>;

	ObjectOf() = default;

	ObjectOf(const ValueType & value) : value(value) {}

	ObjectOf(ValueType & value) : value(std::move(value)) {}

	ObjectOf(const A & a, const B & b, const C & c) : value({ a, b, c }) {}

	ObjectOf(const ObjectOf &) = delete;

	ObjectOf & operator=(const ObjectOf &) = delete;

	ValueType value;
};

template <class TYPE> template <class ... VARGS> REFLEX_INLINE Reflex::ObjectOf<TYPE>::ObjectOf(VARGS && ... args)
	: value(std::forward<VARGS>(args)...)
{
}




//
//macros for library builders

template <class TYPE>
struct Reflex::Detail::ExternalNull < Reflex::ObjectOf <TYPE> >
{
	static inline ConditionalType <kIsTrivial<TYPE>, StaticObject <ObjectOf<TYPE>>, NullType> instance;
};

REFLEX_INSTANTIATE_TYPEID(ObjectOf<bool>);
REFLEX_INSTANTIATE_TYPEID(ObjectOf<UInt8>);
REFLEX_INSTANTIATE_TYPEID(ObjectOf<UInt32>);
REFLEX_INSTANTIATE_TYPEID(ObjectOf<UInt64>);

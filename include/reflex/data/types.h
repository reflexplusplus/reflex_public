
#pragma once

#include "[require].h"




//
//Primary API

namespace Reflex::Data
{

	using Archive = Array <UInt8>;


	using BoolProperty = ObjectOf <bool>;

	using ArrayOfBoolProperty = ObjectOf < Array <bool> >;


	using UInt8Property = ObjectOf <UInt8>;

	using ArchiveObject = ObjectOf <Archive>;

	using BinaryProperty = ArchiveObject;


	using UInt32Property = ObjectOf <UInt32>;

	using UInt64Property = ObjectOf <UInt64>;


	using Int32Property = ObjectOf <Int32>;

	using Int64Property = ObjectOf <Int64>;


	using Float32Property = ObjectOf <Float32>;

	using Float64Property = ObjectOf <Float64>;


	using ArrayOfUInt32Property = ObjectOf < Array <UInt32> >;

	using ArrayOfUInt64Property = ObjectOf < Array <UInt64> >;


	using ArrayOfInt32Property = ObjectOf < Array <Int32> >;

	using ArrayOfInt64Property = ObjectOf < Array <Int64> >;


	using ArrayOfFloat32Property = ObjectOf < Array <Float32> >;

	using ArrayOfFloat64Property = ObjectOf < Array <Float64> >;


	using CStringProperty = ObjectOf <CString>;

	using ArrayOfCStringProperty = ObjectOf < Array <CString> >;


	using WStringProperty = ObjectOf <WString>;

	using ArrayOfWStringProperty = ObjectOf < Array <WString> >;


	using Key32Property = ObjectOf <Key32>;

	using ArrayOfKey32Property = ObjectOf < Array <Key32> >;


	template <class TYPE> using MapOfKey32 = Map <Key32, TYPE>;

	template <class TYPE> using MapOfKey32Property = ObjectOf < MapOfKey32 <TYPE> >;

	using KeyMap = MapOfKey32Property <CString>;

}

template <class TYPE> struct Reflex::SubIndexType < Reflex::Data::MapOfKey32Property <TYPE> > { using Type = Key32; };




//
//impl

REFLEX_EXTERN_NULL(Reflex::Data::BinaryProperty);

REFLEX_EXTERN_NULL(Reflex::Data::ArrayOfUInt32Property);
REFLEX_EXTERN_NULL(Reflex::Data::ArrayOfUInt64Property);
REFLEX_EXTERN_NULL(Reflex::Data::ArrayOfInt32Property);
REFLEX_EXTERN_NULL(Reflex::Data::ArrayOfInt64Property);
REFLEX_EXTERN_NULL(Reflex::Data::ArrayOfFloat32Property);
REFLEX_EXTERN_NULL(Reflex::Data::ArrayOfFloat64Property);

REFLEX_EXTERN_NULL(Reflex::Data::ArrayOfCStringProperty);
REFLEX_EXTERN_NULL(Reflex::Data::ArrayOfWStringProperty);
REFLEX_EXTERN_NULL(Reflex::Data::ArrayOfKey32Property);

REFLEX_EXTERN_NULL(Reflex::Data::KeyMap)
REFLEX_EXTERN_NULL(Reflex::Data::MapOfKey32Property<NullType>)
REFLEX_EXTERN_NULL(Reflex::Data::MapOfKey32Property<bool>)
REFLEX_EXTERN_NULL(Reflex::Data::MapOfKey32Property<UInt32>)
REFLEX_EXTERN_NULL(Reflex::Data::MapOfKey32Property<UInt64>)
REFLEX_EXTERN_NULL(Reflex::Data::MapOfKey32Property<Int32>)
REFLEX_EXTERN_NULL(Reflex::Data::MapOfKey32Property<Int64>)
REFLEX_EXTERN_NULL(Reflex::Data::MapOfKey32Property<Key32>)
REFLEX_EXTERN_NULL(Reflex::Data::MapOfKey32Property<Float32>)
REFLEX_EXTERN_NULL(Reflex::Data::MapOfKey32Property<Float64>)
REFLEX_EXTERN_NULL(Reflex::Data::MapOfKey32Property<Array<UInt8>>)
REFLEX_EXTERN_NULL(Reflex::Data::MapOfKey32Property<WString>)

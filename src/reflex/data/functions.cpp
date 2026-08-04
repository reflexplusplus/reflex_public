#include "reflex/data/format/formats/propertysheet.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::Data)

template <class TYPE> REFLEX_INLINE TYPE GetValueWithFallback(const Object & object, Key32 id, TYPE fallback)
{
	Reflex::Detail::Initialiser < ObjectOf <TYPE> > fallback_object;

	fallback_object->value = fallback;

	return object.QueryProperty< ObjectOf <TYPE> >(id, Cast< ObjectOf <TYPE> >(fallback_object.Adr()))->value;
}

template <class TYPE> REFLEX_INLINE void UnsetArrayProperty(Object & object, Key32 id)
{
	typedef ObjectOf < Array <TYPE> > Property;

	object.UnsetProperty<Property>(id);
}

template <class TYPE> REFLEX_INLINE void SetArrayProperty(Object & object, Key32 id, const ArrayView <TYPE> & value)
{
	typedef ObjectOf < Array <TYPE> > Property;

	//if (value)
	{
		object.SetProperty(id, REFLEX_CREATE(Property, value));
	}
	//else
	{
		//object.UnsetProperty<Property>(id);
	}
}

template <class TYPE> REFLEX_INLINE ArrayView <TYPE> GetArrayProperty(const Object & object, Key32 id)
{
	return GetProperty< ObjectOf < Array<TYPE> > >(object, id)->value;
}

template <class TYPE> REFLEX_INLINE ArrayView <TYPE> GetArrayPropertyWithFallback(const Object & object, Key32 id, const ArrayView <TYPE> & fallback)
{
	typedef ObjectOf < Array<TYPE> > Type;

	if (auto pobject = RemoveConst(object).QueryProperty(MakeAddress<Type>(id), 0))
	{
		return Cast<Type>(pobject)->value;
	}
	else
	{
		return fallback;
	}
}

REFLEX_END_INTERNAL

void Reflex::Data::UnsetPropertySet(Object & object, Key32 id)
{
	object.UnsetProperty<PropertySet>(id);
}

void Reflex::Data::SetPropertySet(Object & object, Key32 id, TRef <PropertySet> child)
{
	object.SetProperty(id, child);
}

Reflex::TRef <Reflex::Data::PropertySet> Reflex::Data::AcquirePropertySet(PropertySet & propertyset, const ArrayView <Key32> & path)
{
	TRef <PropertySet> rtn = propertyset;

	for (auto & i : path) rtn = Detail::AcquireProperty<PropertySet>(rtn, i);

	return rtn;
}

Reflex::ConstTRef <Reflex::Data::PropertySet> Reflex::Data::GetPropertySet(const Object & object, Key32 id)
{
	return GetProperty<PropertySet>(object, id);
}

Reflex::ConstTRef <Reflex::Data::PropertySet> Reflex::Data::GetPropertySet(const PropertySet & propertyset, const ArrayView <Key32> & path)
{
	ConstTRef <PropertySet> rtn = propertyset;

	for (auto & i : path) rtn = GetProperty<PropertySet>(rtn, i);

	return rtn;
}

void Reflex::Data::UnsetBool(Object & object, Key32 key)
{
	object.UnsetProperty<BoolProperty>(key);
}

void Reflex::Data::SetBool(Object & object, Key32 key, bool value)
{
	object.SetProperty(key, REFLEX_CREATE(BoolProperty, value));
}

bool Reflex::Data::GetBool(const Object & object, Key32 id, bool fallback)
{
	return GetValueWithFallback(object, id, fallback);
}

void Reflex::Data::UnsetUInt8(Object & object, Key32 key)
{
	object.UnsetProperty<UInt8Property>(key);
}

void Reflex::Data::SetUInt8(Object & object, Key32 key, UInt8 value)
{
	object.SetProperty(key, REFLEX_CREATE(UInt8Property, value));
}

Reflex::UInt8 Reflex::Data::GetUInt8(const Object & object, Key32 id, UInt8 fallback)
{
	return GetValueWithFallback(object, id, fallback);
}

void Reflex::Data::UnsetUInt32(Object & object, Key32 key)
{
	object.UnsetProperty<UInt32Property>(key);
}

void Reflex::Data::SetUInt32(Object & object, Key32 key, UInt32 value)
{
	object.SetProperty(key, REFLEX_CREATE(UInt32Property, value));
}

Reflex::UInt32 Reflex::Data::GetUInt32(const Object & object, Key32 id, UInt32 fallback)
{
	return GetValueWithFallback(object, id, fallback);
}

void Reflex::Data::UnsetUInt64(Object & object, Key32 key)
{
	object.UnsetProperty<UInt64Property>(key);
}

void Reflex::Data::SetUInt64(Object & object, Key32 key, UInt64 value)
{
	object.SetProperty(key, REFLEX_CREATE(UInt64Property, value));
}

Reflex::UInt64 Reflex::Data::GetUInt64(const Object & object, Key32 id, UInt64 fallback)
{
	return GetValueWithFallback(object, id, fallback);
}

void Reflex::Data::UnsetInt32(Object & object, Key32 key)
{
	object.UnsetProperty<Int32Property>(key);
}

void Reflex::Data::SetInt32(Object & object, Key32 key, Int32 value)
{
	object.SetProperty(key, REFLEX_CREATE(Int32Property, value));
}

Reflex::Int32 Reflex::Data::GetInt32(const Object & object, Key32 id, Int32 fallback)
{
	return GetValueWithFallback(object, id, fallback);
}

void Reflex::Data::UnsetInt64(Object & object, Key32 key)
{
	object.UnsetProperty<Int64Property>(key);
}

void Reflex::Data::SetInt64(Object & object, Key32 key, Int64 value)
{
	object.SetProperty(key, REFLEX_CREATE(Int64Property, value));
}

Reflex::Int64 Reflex::Data::GetInt64(const Object & object, Key32 id, Int64 fallback)
{
	return GetValueWithFallback(object, id, fallback);
}

void Reflex::Data::UnsetFloat32(Object & object, Key32 key)
{
	object.UnsetProperty<Float32Property>(key);
}

void Reflex::Data::SetFloat32(Object & object, Key32 key, Float32 value)
{
	object.SetProperty(key, REFLEX_CREATE(Float32Property, value));
}

Reflex::Float32 Reflex::Data::GetFloat32(const Object & object, Key32 id, Float32 fallback)
{
	return GetValueWithFallback(object, id, fallback);
}

void Reflex::Data::UnsetFloat64(Object & object, Key32 key)
{
	object.UnsetProperty<Float64Property>(key);
}

void Reflex::Data::SetFloat64(Object & object, Key32 key, Float64 value)
{
	object.SetProperty(key, REFLEX_CREATE(Float64Property, value));
}

Reflex::Float64 Reflex::Data::GetFloat64(const Object & object, Key32 id, Float64 fallback)
{
	return GetValueWithFallback(object, id, fallback);
}

void Reflex::Data::UnsetKey32(Object & object, Key32 key)
{
	object.UnsetProperty<Key32Property>(key);
}

void Reflex::Data::SetKey32(Object & object, Key32 key, Key32 attribute)
{
	object.SetProperty(key, REFLEX_CREATE(Key32Property, attribute));
}

Reflex::Key32 Reflex::Data::GetKey32(const Object & object, Key32 id, Key32 fallback)
{
	return GetValueWithFallback(object, id, fallback);
}

void Reflex::Data::UnsetBinary(Object & object, Key32 key)
{
	object.UnsetProperty<ArchiveObject>(key);
}

void Reflex::Data::SetBinary(Object & object, Key32 key, const Data::Archive::View & value)
{
	SetArrayProperty(object, key, value);
}

Reflex::Data::Archive::View Reflex::Data::GetBinary(const Object & object, Key32 key)
{
	return GetArrayProperty<UInt8>(object, key);
}

Reflex::Data::Archive::View Reflex::Data::GetBinary(const Object & object, Key32 key, const Archive::View & fallback)
{
	return GetArrayPropertyWithFallback(object, key, fallback);
}

void Reflex::Data::UnsetCString(Object & object, Key32 id)
{
	object.UnsetProperty<CStringProperty>(id);
}

void Reflex::Data::SetCString(Object & object, Key32 id, const CString::View & value)
{
	SetArrayProperty(object, id, value);
}

Reflex::CString::View Reflex::Data::GetCString(const Object & object, Key32 id)
{
	return GetArrayProperty<char>(object, id);
}

Reflex::CString::View Reflex::Data::GetCString(const Object & object, Key32 id, const CString::View & fallback)
{
	return GetArrayPropertyWithFallback(object, id, fallback);
}

void Reflex::Data::UnsetWString(Object & object, Key32 id)
{
	object.UnsetProperty<WStringProperty>(id);
}

void Reflex::Data::SetWString(Object & object, Key32 id, const WString::View & value)
{
	SetArrayProperty(object, id, value);
}

Reflex::WString::View Reflex::Data::GetWString(const Object & object, Key32 id)
{
	return GetArrayProperty<WChar>(object, id);
}

Reflex::WString::View Reflex::Data::GetWString(const Object & object, Key32 id, const WString::View & fallback)
{
	return GetArrayPropertyWithFallback(object, id, fallback);
}

void Reflex::Data::UnsetPropertySetArray(PropertySet & propertyset, Key32 id)
{
	propertyset.UnsetProperty<PropertySetArray>(id);
}

Reflex::TRef <Reflex::Data::PropertySetArray> Reflex::Data::AcquirePropertySetArray(PropertySet & propertyset, Key32 id)
{
	return Detail::AcquireProperty<PropertySetArray>(propertyset, id);
}

Reflex::TRef <Reflex::Data::PropertySet> Reflex::Data::AddPropertySet(PropertySetArray & array)
{
	return array.value.Push(New<PropertySet>());
}

Reflex::ArrayView < Reflex::ConstReference <Reflex::Data::PropertySet> > Reflex::Data::GetPropertySetArray(const PropertySet & propertyset, Key32 id)
{
	auto view = ToView(GetProperty<PropertySetArray>(propertyset, id)->value);

	return Reinterpret < ArrayView < ConstReference <PropertySet> > >(view);
}

void Reflex::Data::UnsetUInt32Array(Object & object, Key32 id)
{
	UnsetArrayProperty<UInt32>(object, id);
}

void Reflex::Data::SetUInt32Array(Object & object, Key32 key, const ArrayView <UInt32> & values)
{
	object.SetProperty(key, REFLEX_CREATE(ArrayOfUInt32Property, values));
}

Reflex::ArrayView <Reflex::UInt32> Reflex::Data::GetUInt32Array(const Object & object, Key32 key)
{
	return GetProperty<ArrayOfUInt32Property>(object, key)->value;
}

Reflex::ArrayView <Reflex::UInt64> Reflex::Data::GetUInt64Array(const Object & object, Key32 key)
{
	return GetProperty<ArrayOfUInt64Property>(object, key)->value;
}

void Reflex::Data::SetUInt64Array(Object & object, Key32 key, const ArrayView <UInt64> & values)
{
	object.SetProperty(key, REFLEX_CREATE(ArrayOfUInt64Property, values));
}

Reflex::ArrayView <Reflex::Int32> Reflex::Data::GetInt32Array(const Object & object, Key32 key)
{
	return GetProperty<ArrayOfInt32Property>(object, key)->value;
}

void Reflex::Data::SetInt32Array(Object & object, Key32 key, const ArrayView <Int32> & values)
{
	object.SetProperty(key, REFLEX_CREATE(ArrayOfInt32Property, values));
}

Reflex::ArrayView <Reflex::Int64> Reflex::Data::GetInt64Array(const Object & object, Key32 key)
{
	return GetProperty<ArrayOfInt64Property>(object, key)->value;
}

void Reflex::Data::SetInt64Array(Object & object, Key32 key, const ArrayView <Int64> & values)
{
	object.SetProperty(key, REFLEX_CREATE(ArrayOfInt64Property, values));
}

void Reflex::Data::UnsetFloat32Array(Object & object, Key32 key)
{
	object.UnsetProperty<ArrayOfFloat32Property>(key);
}

void Reflex::Data::SetFloat32Array(Object & object, Key32 key, const ArrayView <Float32> & values)
{
	object.SetProperty(key, REFLEX_CREATE(ArrayOfFloat32Property, values));
}

Reflex::ArrayView <Reflex::Float32> Reflex::Data::GetFloat32Array(const Object & object, Key32 key)
{
	return GetProperty<ArrayOfFloat32Property>(object, key)->value;
}

void Reflex::Data::UnsetCStringArray(Object & object, Key32 key)
{
	object.UnsetProperty<ArrayOfCStringProperty>(key);
}

void Reflex::Data::SetCStringArray(Object & object, Key32 key, const ArrayView <CString> & values)
{
	REFLEX_ASSERT(!object.QueryProperty<ArrayOfCStringProperty>(key));	//TEMP CHECK, for some reason was using AcquireProperty semantic

	object.SetProperty(key, New<ArrayOfCStringProperty>(values));
}

Reflex::ArrayView <Reflex::CString> Reflex::Data::GetCStringArray(const Object & object, Key32 id)
{
	return GetProperty<ArrayOfCStringProperty>(object, id)->value;
}

void Reflex::Data::UnsetWStringArray(Object & object, Key32 key)
{
	object.UnsetProperty<ArrayOfWStringProperty>(key);
}

void Reflex::Data::SetWStringArray(Object & object, Key32 key, const ArrayView <WString> & values)
{
	REFLEX_ASSERT(!object.QueryProperty<ArrayOfCStringProperty>(key));	//TEMP CHECK, for some reason was using AcquireProperty semantic

	object.SetProperty(key, New<ArrayOfWStringProperty>(values));
}

Reflex::ArrayView <Reflex::WString> Reflex::Data::GetWStringArray(const Object & object, Key32 id)
{
	return GetProperty<ArrayOfWStringProperty>(object, id)->value;
}

void Reflex::Data::SetKey32Array(Object & object, Key32 key, const ArrayView <Key32> & values)
{
	object.SetProperty(key, REFLEX_CREATE(ArrayOfKey32Property, values));
}

Reflex::ArrayView <Reflex::Key32> Reflex::Data::GetKey32Array(const Object & object, Key32 key)
{
	return GetProperty<ArrayOfKey32Property>(object, key)->value;
}

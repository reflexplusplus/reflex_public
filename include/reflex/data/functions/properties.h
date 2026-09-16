#pragma once

#include "../propertyset.h"
#include "../types.h"




//
//Primary API

namespace Reflex::Data
{

	template <class TYPE> using ObjectArray = ObjectOf < Array < Reference <TYPE> > >;

	using PropertySetArray = ObjectArray <PropertySet>;


	void UnsetPropertySet(Object & object, Key32 id);

	void SetPropertySet(Object & object, Key32 id, WillRetain <PropertySet> child);

	AlreadyRetained <PropertySet> AcquirePropertySet(PropertySet & propertyset, Key32 id);

	ConstAlreadyRetained <PropertySet> GetPropertySet(const Object & object, Key32 id);


	AlreadyRetained <PropertySet> AcquirePropertySet(PropertySet & propertyset, ArrayView <Key32> path);

	ConstAlreadyRetained <PropertySet> GetPropertySet(const PropertySet & propertyset, ArrayView <Key32> path);


	void UnsetPropertySetArray(PropertySet & propertyset, Key32 id);

	AlreadyRetained <PropertySetArray> AcquirePropertySetArray(PropertySet & propertyset, Key32 id);

	AlreadyRetained <PropertySet> AddPropertySet(PropertySetArray & array);

	ArrayView < ConstReference <PropertySet> > GetPropertySetArray(const PropertySet & propertyset, Key32 id);


	void UnsetBool(Object & object, Key32 key);

	void SetBool(Object & object, Key32 key, bool value);

	bool GetBool(const Object & object, Key32 id, bool fallback = false);


	void UnsetUInt8(Object & object, Key32 key);

	void SetUInt8(Object & object, Key32 key, UInt8 value);

	UInt8 GetUInt8(const Object & object, Key32 id, UInt8 fallback = 0);


	void UnsetUInt32(Object & object, Key32 key);

	void SetUInt32(Object & object, Key32 id, UInt32 value);

	UInt32 GetUInt32(const Object & object, Key32 id, UInt32 fallback = 0);


	void UnsetUInt64(Object & object, Key32 key);

	void SetUInt64(Object & object, Key32 id, UInt64 value);

	UInt64 GetUInt64(const Object & object, Key32 id, UInt64 fallback = 0);


	void UnsetInt32(Object & object, Key32 key);

	void SetInt32(Object & object, Key32 id, Int32 value);

	Int32 GetInt32(const Object & object, Key32 id, Int32 fallback = 0);


	void UnsetInt64(Object & object, Key32 key);

	void SetInt64(Object & object, Key32 id, Int64 value);

	Int64 GetInt64(const Object & object, Key32 id, Int64 fallback = 0);


	void UnsetKey32(Object & object, Key32 key);

	void SetKey32(Object & object, Key32 key, Key32 attribute);

	Key32 GetKey32(const Object & object, Key32 id, Key32 fallback = {});


	void UnsetFloat32(Object & object, Key32 key);

	void SetFloat32(Object & object, Key32 key, Float32 value);

	Float32 GetFloat32(const Object & object, Key32 id, Float32 fallback = 0.0f);


	void UnsetFloat64(Object & object, Key32 key);

	void SetFloat64(Object & object, Key32 key, Float64 value);

	Float64 GetFloat64(const Object & object, Key32 id, Float64 fallback = 0.0);


	void UnsetBinary(Object & object, Key32 key);

	void SetBinary(Object & object, Key32 key, Archive::View value);

	Archive::View GetBinary(const Object & object, Key32 key);

	Archive::View GetBinary(const Object & object, Key32 key, Archive::View fallback);


	void UnsetCString(Object & object, Key32 key);

	void SetCString(Object & object, Key32 key, CString::View value);

	CString::View GetCString(const Object & object, Key32 key);

	CString::View GetCString(const Object & object, Key32 key, CString::View fallback);

	CString::View GetCString(const Object & object, Key32 key, const char * fallback);


	void UnsetWString(Object & object, Key32 key);

	void SetWString(Object & object, Key32 key, WString::View value);

	WString::View GetWString(const Object & object, Key32 key);

	WString::View GetWString(const Object & object, Key32 key, WString::View fallback);

	WString::View GetWString(const Object & object, Key32 key, const WChar * fallback);


	void UnsetUInt32Array(Object & object, Key32 key);

	void SetUInt32Array(Object & object, Key32 key, ArrayView <UInt32> values);

	ArrayView <UInt32> GetUInt32Array(const Object & object, Key32 key);


	void SetUInt64Array(Object & object, Key32 key, ArrayView <UInt64> values);

	ArrayView <UInt64> GetUInt64Array(const Object & object, Key32 key);


	void SetInt32Array(Object & object, Key32 key, ArrayView <Int32> values);

	ArrayView <Int32> GetInt32Array(const Object & object, Key32 key);


	void SetInt64Array(Object & object, Key32 key, ArrayView <Int64> values);

	ArrayView <Int64> GetInt64Array(const Object & object, Key32 key);


	void UnsetFloat32Array(Object & object, Key32 key);

	void SetFloat32Array(Object & object, Key32 key, ArrayView <Float32> values);

	ArrayView <Float32> GetFloat32Array(const Object & object, Key32 key);


	void SetKey32Array(Object & object, Key32 id, ArrayView <Key32> values);

	ArrayView <Key32> GetKey32Array(const Object & object, Key32 id);


	void UnsetCStringArray(Object & object, Key32 key);

	void SetCStringArray(Object & object, Key32 key, ArrayView <CString> values);

	ArrayView <CString> GetCStringArray(const Object & object, Key32 key);


	void UnsetWStringArray(Object & object, Key32 key);

	void SetWStringArray(Object & object, Key32 key, ArrayView <WString> values);

	ArrayView <WString> GetWStringArray(const Object & object, Key32 key);



	//prohibit

	Archive::View GetBinary(const Object & object, Key32 key, Archive && fallback) = delete;
	CString::View GetCString(const Object & object, Key32 key, CString && fallback) = delete;
	WString::View GetWString(const Object & object, Key32 key, WString && fallback) = delete;

}

REFLEX_EXTERN_NULL(Reflex::Data::PropertySetArray);




//
//impl

inline Reflex::AlreadyRetained <Reflex::Data::PropertySet> Reflex::Data::AcquirePropertySet(PropertySet & propertyset, Key32 id)
{
	return AcquirePropertySet(propertyset, ToView(id));
}

inline Reflex::CString::View Reflex::Data::GetCString(const Object & object, Key32 key, const char * fallback)
{
	return GetCString(object, key, ToView(fallback)); 
}

inline Reflex::WString::View Reflex::Data::GetWString(const Object & object, Key32 key, const WChar * fallback)
{
	return GetWString(object, key, ToView(fallback));
}

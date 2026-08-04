#include "rbo.h"




//
//impl

#define OBJECT_T(TYPE) REFLEX_TYPEID(ObjectOf<TYPE>)

#define REGISTER_VALUE(TYPE) format.SetTypeHandler({OBJECT_T(TYPE), PersistentID<TYPE>::id, REFLEX_STRINGIFY(TYPE), &TypeEncoder<TYPE>::SerializeValue, &TypeEncoder<TYPE>::DeserializeValue, &Reflex::Data::TypeEncoder<TYPE>::CompareValue, &GetNull<ObjectOf<TYPE>> })
#define REGISTER_ARRAY(TYPE) format.SetTypeHandler({OBJECT_T(Array<TYPE>), PersistentID<Array<TYPE>>::id, "Array@" REFLEX_STRINGIFY(TYPE), &TypeEncoder<TYPE>::SerializeArray, &TypeEncoder<TYPE>::DeserializeArray, &Reflex::Data::TypeEncoder<TYPE>::CompareArray, &GetNull<ObjectOf<Array<TYPE>>> })

#define DECLARE_PERSISTENT_TYPEID(TYPE,ID) template <> struct PersistentID <TYPE> { static constexpr UInt8 id = ID; }

#define DECLARE_PERSISTENT_ARRAY_TYPEID(TYPE,ID) template <> struct PersistentID < Array <TYPE> > { static constexpr UInt8 id = ID ; }
#define DECLARE_PERSISTENT_KEYMAP_TYPEID(TYPE,ID) template <> struct PersistentID < MapOfKey32 <TYPE> > { static constexpr UInt8 id = ID ; }

REFLEX_BEGIN_INTERNAL(Reflex::Data)

template <class TYPE> struct PersistentID { };

template <class TYPE> struct PersistentID < Array <TYPE> > { static constexpr UInt8 id = PersistentID<TYPE>::id + 64; };

template <class TYPE> inline TRef <Object> GetNull() { return Null<TYPE>(); }

DECLARE_PERSISTENT_TYPEID(bool, 'b');		//b

DECLARE_PERSISTENT_TYPEID(UInt8, 'v');
DECLARE_PERSISTENT_TYPEID(UInt16, 'V');
DECLARE_PERSISTENT_TYPEID(UInt32, 'u');
DECLARE_PERSISTENT_TYPEID(UInt64, 'U');

DECLARE_PERSISTENT_TYPEID(Int8, 's');
DECLARE_PERSISTENT_TYPEID(Int16, 'S');
DECLARE_PERSISTENT_TYPEID(Int32, 'i');
DECLARE_PERSISTENT_TYPEID(Int64, 'I');

DECLARE_PERSISTENT_TYPEID(Float32, 'f');
DECLARE_PERSISTENT_TYPEID(Float64, 'F');

DECLARE_PERSISTENT_TYPEID(Key32, 'k');

DECLARE_PERSISTENT_TYPEID(CString, 'c');
DECLARE_PERSISTENT_TYPEID(WString, 'w');

DECLARE_PERSISTENT_KEYMAP_TYPEID(NullType, 191);
DECLARE_PERSISTENT_KEYMAP_TYPEID(bool, 192);
DECLARE_PERSISTENT_KEYMAP_TYPEID(UInt32, 194);
DECLARE_PERSISTENT_KEYMAP_TYPEID(UInt64, 195);
DECLARE_PERSISTENT_KEYMAP_TYPEID(Int32, 198);
DECLARE_PERSISTENT_KEYMAP_TYPEID(Int64, 199);
DECLARE_PERSISTENT_KEYMAP_TYPEID(Float32, 200);
DECLARE_PERSISTENT_KEYMAP_TYPEID(Float64, 201);
DECLARE_PERSISTENT_KEYMAP_TYPEID(Key32, 202);

DECLARE_PERSISTENT_KEYMAP_TYPEID(Archive, '{' + 'a');
DECLARE_PERSISTENT_KEYMAP_TYPEID(CString, '{');
DECLARE_PERSISTENT_KEYMAP_TYPEID(WString, ('{' + ('w' - 'c')));

DECLARE_PERSISTENT_ARRAY_TYPEID(UInt8, 'a');
DECLARE_PERSISTENT_ARRAY_TYPEID(UInt32, '[');
DECLARE_PERSISTENT_ARRAY_TYPEID(Key32, '#');

DECLARE_PERSISTENT_ARRAY_TYPEID(CString, 'C');
DECLARE_PERSISTENT_ARRAY_TYPEID(WString, 'W');

//template <class TYPE> static TRef <Object> CreateValue()
//{
//	return REFLEX_CREATE(ObjectOf<TYPE>);
//}

template <class TYPE>
struct TypeEncoder
{
	typedef ObjectOf <TYPE> ObjectOfType;

	static void SerializeValue(const SerializableFormat & format, Archive & stream, const Object & object)
	{
		Data::Serialize(stream, *Cast<ObjectOfType>(object));
	}

	static TRef <Object> DeserializeValue(const SerializableFormat & format, Archive::View & stream)
	{
		auto object = REFLEX_CREATE(ObjectOfType);

		Data::Deserialize(stream, *object);

		return object;
	}

	static void SerializeArray(const SerializableFormat & format, Archive & stream, const Object & object)
	{
		Data::Serialize(stream, *Cast<ObjectOf<Array<TYPE>>>(object));
	}

	static TRef <Object> DeserializeArray(const SerializableFormat & format, Archive::View & stream)
	{
		auto object = REFLEX_CREATE(ObjectOf<Array<TYPE>>);

		Data::Deserialize(stream, *object);

		return object;
	}

	static bool CompareValue(const Detail::PropertySetFormat & format, const Object & a, const Object & b)
	{
		return Cast<ObjectOfType>(a)->value == Cast<ObjectOfType>(b)->value;
	}

	static bool CompareArray(const Detail::PropertySetFormat & format, const Object & a, const Object & b)
	{
		return Cast< ObjectOf < Array <TYPE> > >(a)->value == Cast< ObjectOf < Array <TYPE> > >(b)->value;
	}
};

template <>
struct TypeEncoder <Key32>
{
	static void Serialize(Archive & stream, Key32 value)
	{
		Data::Serialize(stream, value.value - kHashSeed);
	}

	static void Deserialize(Archive::View & stream, Key32 & value)
	{
		value = Data::Deserialize<UInt32>(stream) + kHashSeed;
	}

	static void SerializeValue(const SerializableFormat & format, Archive & stream, const Object & object)
	{
		Serialize(stream, Cast<Key32Property>(object)->value);
	}

	static TRef <Object> DeserializeValue(const SerializableFormat & format, Archive::View & stream)
	{
		auto object = REFLEX_CREATE(Key32Property);

		Deserialize(stream, object->value);

		return object;
	}

	static void SerializeArray(const SerializableFormat & format, Archive & stream, const Object & object)
	{
		auto & values = Cast<ArrayOfKey32Property>(object)->value;

		Data::Serialize(stream, values.GetSize());

		for (auto & i : values) Serialize(stream, i);
	}

	static TRef <Object> DeserializeArray(const SerializableFormat & format, Archive::View & stream)
	{
		auto object = REFLEX_CREATE(ArrayOfKey32Property);

		object->value.SetSize(Data::Deserialize<UInt32>(stream));

		for (auto & i : object->value) Deserialize(stream, i);

		return object;
	}

	static bool CompareValue(const Detail::PropertySetFormat & format, const Object & a, const Object & b)
	{
		return Cast<Key32Property>(a)->value == Cast<Key32Property>(b)->value;
	}

	static bool CompareArray(const Detail::PropertySetFormat & format, const Object & a, const Object & b)
	{
		return Cast<ArrayOfKey32Property>(a)->value == Cast<ArrayOfKey32Property>(b)->value;
	}
};

template <>
struct TypeEncoder <WString>
{
	static void SerializeValue(const SerializableFormat & format, Archive & stream, const Object & object)
	{
		SerializeUTF8(stream, Cast<WStringProperty>(object)->value);
	}

	static TRef <Object> DeserializeValue(const SerializableFormat & format, Archive::View & stream)
	{
		auto object = REFLEX_CREATE(WStringProperty);

		DeserializeUTF8(stream, object->value);

		return object;
	}

	static void SerializeArray(const SerializableFormat & format, Archive & stream, const Object & object)
	{
		auto & values = Cast<ArrayOfWStringProperty>(object)->value;

		Serialize(stream, values.GetSize());

		for (auto & i : values) SerializeUTF8(stream, i);
	}

	static TRef <Object> DeserializeArray(const SerializableFormat & format, Archive::View & stream)
	{
		auto strings = REFLEX_CREATE(ArrayOfWStringProperty);

		strings->value.SetSize(Data::Deserialize<UInt32>(stream));

		for (auto & i : strings->value) DeserializeUTF8(stream, i);

		return strings;
	}

	static bool CompareValue(const Detail::PropertySetFormat & format, const Object & a, const Object & b)
	{
		return Cast<WStringProperty>(a)->value == Cast<WStringProperty>(b)->value;
	}

	static bool CompareArray(const Detail::PropertySetFormat & format, const Object & a, const Object & b)
	{
		return Cast<ArrayOfWStringProperty>(a)->value == Cast<ArrayOfWStringProperty>(b)->value;
	}
};

template <>
struct TypeEncoder < MapOfKey32<WString> >
{
	typedef ObjectOf < MapOfKey32<WString> > PropertyType;

	static void SerializeValue(const SerializableFormat & format, Archive & stream, const Object & object)
	{
		auto & map = Cast<PropertyType>(object)->value;

		Serialize(stream, map.GetSize());

		for (auto & i : map)
		{
			Serialize(stream, i.key);

			SerializeUTF8(stream, i.value);
		}
	}

	static TRef <Object> DeserializeValue(const SerializableFormat & format, Archive::View & stream)
	{
		auto object = REFLEX_CREATE(PropertyType);

		auto size = Deserialize<UInt32>(stream);

		WString value;

		REFLEX_LOOP(idx, size)
		{
			auto key = Deserialize<Key32>(stream);

			DeserializeUTF8(stream, value);

			object->value.Set(key, std::move(value));
		}

		return object;
	}

	static bool CompareValue(const Detail::PropertySetFormat & format, const Object & a, const Object & b)
	{
		return Cast<PropertyType>(a)->value == Cast<PropertyType>(b)->value;
	}
};

REFLEX_END_INTERNAL

void Reflex::Data::InitialiseStandardPropertySet(PropertySetFormatImpl & format)
{
	format.SetTypeHandler
	({
		REFLEX_TYPEID(Object),	//this is to support JSON null
		char(0),
		"Object",
		[](const SerializableFormat & format, Archive & stream, const Object & object)
		{
		},
		[](const SerializableFormat & format, Archive::View & stream) -> TRef <Object>
		{
			return Object::null;
		},
		[](const Detail::PropertySetFormat & format, const Object & a, const Object & b)
		{
			return &a == &b;
		},
		&GetNull<Object>
	});

	format.SetTypeHandler
	({
		REFLEX_TYPEID(PropertySet),
		'@',
		"Data::PropertySet",
		[](const SerializableFormat & format, Archive & stream, const Object & object)
		{
			Cast<PropertySetFormatImpl>(format)->SerializePropertySetImpl(stream, Cast<PropertySet>(object));
		},
		[](const SerializableFormat & format, Archive::View & stream) -> TRef <Object>
		{
			auto object = REFLEX_CREATE(PropertySet);

			Cast<PropertySetFormatImpl>(format)->DeserializePropertySetImpl(stream, *object);

			return object;
		},
		reinterpret_cast<decltype(Detail::PropertySetFormat::TypeHandler::compare)>(&PropertySetFormatImpl::CompareDynamic),
		&GetNull<PropertySet>
	});

	format.SetTypeHandler
	({
		REFLEX_TYPEID(PropertySetArray),
		'@' + 64,
		"Array@Data::PropertySet",
		[](const SerializableFormat & format, Archive & stream, const Object & object)
		{
			auto & a = Cast<PropertySetArray>(object)->value;

			Data::Serialize(stream, a.GetSize());

			for (auto & i : a) Cast<PropertySetFormatImpl>(format)->SerializePropertySetImpl(stream, i);
		},
		[](const SerializableFormat & format, Archive::View & stream) -> TRef <Object>
		{
			auto object = REFLEX_CREATE(PropertySetArray);

			REFLEX_LOOP(idx, Data::Deserialize<UInt32>(stream)) Cast<PropertySetFormatImpl>(format)->DeserializePropertySetImpl(stream, object->value.Push(REFLEX_CREATE(PropertySet)));

			return object;
		},
		reinterpret_cast<decltype(Detail::PropertySetFormat::TypeHandler::compare)>(&PropertySetFormatImpl::CompareArraysOfDynamics),
		&GetNull<PropertySetArray>
	});

	REGISTER_VALUE(bool);
	//REGISTER_ARRAY(bool);

	REGISTER_VALUE(UInt8);
	REGISTER_ARRAY(UInt8);

	REGISTER_VALUE(UInt16);
	//REGISTER_ARRAY(UInt16);

	REGISTER_VALUE(UInt32);
	REGISTER_ARRAY(UInt32);

	REGISTER_VALUE(UInt64);
	REGISTER_ARRAY(UInt64);

	REGISTER_VALUE(Int8);
	//REGISTER_ARRAY(Int8);

	REGISTER_VALUE(Int16);
	//REGISTER_ARRAY(Int16);

	REGISTER_VALUE(Int32);
	REGISTER_ARRAY(Int32);

	REGISTER_VALUE(Int64);
	REGISTER_ARRAY(Int64);

	REGISTER_VALUE(Float32);
	REGISTER_ARRAY(Float32);

	REGISTER_VALUE(Float64);
	REGISTER_ARRAY(Float64);

	REGISTER_VALUE(Key32);
	REGISTER_ARRAY(Key32);

	REGISTER_VALUE(CString);
	REGISTER_ARRAY(CString);

	REGISTER_VALUE(WString);
	REGISTER_ARRAY(WString);

	REGISTER_VALUE(MapOfKey32<NullType>);
	REGISTER_VALUE(MapOfKey32<bool>);
	REGISTER_VALUE(MapOfKey32<UInt32>);
	REGISTER_VALUE(MapOfKey32<UInt64>);
	REGISTER_VALUE(MapOfKey32<Int32>);
	REGISTER_VALUE(MapOfKey32<Int64>);
	REGISTER_VALUE(MapOfKey32<Float32>);
	REGISTER_VALUE(MapOfKey32<Float64>);
	REGISTER_VALUE(MapOfKey32<Key32>);
	REGISTER_VALUE(MapOfKey32<Archive>);
	REGISTER_VALUE(MapOfKey32<CString>);
	REGISTER_VALUE(MapOfKey32<WString>);

	//constexpr auto a = PersistentID<Data::Archive>::id;
	//constexpr auto b = PersistentID<MapOfKey32<bool>>::id;
}

//TODO dont do this, the RTTID is not available at static init

//const Reflex::Data::PropertySetFormat::TypeHandler Reflex::Data::kPropertySetUInt8 = MAKE_VALUE_TYPE(UInt8);
//const Reflex::Data::PropertySetFormat::TypeHandler Reflex::Data::kPropertySetInt32 = MAKE_VALUE_TYPE(Int32);
//const Reflex::Data::PropertySetFormat::TypeHandler Reflex::Data::kPropertySetFloat32 = MAKE_VALUE_TYPE(Float32);
//const Reflex::Data::PropertySetFormat::TypeHandler Reflex::Data::kPropertySetKey32 = MAKE_VALUE_TYPE(Key32);

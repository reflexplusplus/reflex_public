#include "functions.h"

#include "formats/defines.h"
#include "formats/rbo.h"

#ifndef REFLEX_MINIMAL
#include "formats/propertysheet/standardformatimpl.h"
#include "formats/json.h"
#include "formats/riff.h"
#include "formats/xml.h"
#endif




REFLEX_INSTANTIATE_DEFAULT_ALLOCATOR;

REFLEX_BEGIN_INTERNAL(Reflex::Data)

struct NullPropertySet : public Data::PropertySet
{
	void OnUnsetProperty(Address address) override {}
	void OnQueryProperty(Address address, Object *& ptr) const override {}
	void OnSetProperty(Address address, Object & object) override
	{
		AutoRelease(object);

		File::output.Error("Data::PropertySet::OnSetProperty on null discarded");
	}
};

struct NullFormat : public SerializableFormat
{
	DeserializeError OnDeserialize(Archive::View & stream, PropertySet & data, UInt32 options) const override { return kDeserializeErrorUnsupportedVersion; }
	void OnSerialize(Archive & stream, const PropertySet & data) const override {}
	bool SupportsType(TypeID type_id) const override { return false; }
	void OnReset(PropertySet & data) const override {}
};

struct NullStreamable : public iStreamable
{
	NullStreamable() : iStreamable(0) {}

	void OnReset(Key32 context) override {}
	void OnRestore(Archive::View & stream, Key32 context) override {}
	void OnStore(Archive & stream) const override {}
};

struct Library
{
	template <class TYPE> using NullImpl = Reflex::Detail::StaticObject <TYPE>;

	Library();


	NullImpl <NullPropertySet> null_propertyset;

	NullFormat null_format;

	NullStreamable null_streamable;


	NullImpl <BinaryProperty> null_binary;
	NullImpl <PropertySetArray> null_propertysets;
	NullImpl <ArrayOfUInt32Property> null_uint32s;
	NullImpl <ArrayOfUInt64Property> null_uint64s;
	NullImpl <ArrayOfInt32Property> null_int32s;
	NullImpl <ArrayOfInt64Property> null_int64s;
	NullImpl <ArrayOfFloat32Property> null_float32s;
	NullImpl <ArrayOfFloat64Property> null_float64s;
	NullImpl <ArrayOfKey32Property> null_keys;
	NullImpl <ArrayOfCStringProperty> null_cstrings;
	NullImpl <ArrayOfWStringProperty> null_wstrings;

	NullImpl <MapOfKey32Property<NullType>> null_nulltype_keymap;
	NullImpl <MapOfKey32Property<bool>> null_bool_keymap;
	NullImpl <MapOfKey32Property<UInt32>> null_uint32_keymap;
	NullImpl <MapOfKey32Property<UInt64>> null_uint64_keymap;
	NullImpl <MapOfKey32Property<Int32>> null_int32_keymap;
	NullImpl <MapOfKey32Property<Int64>> null_int64_keymap;
	NullImpl <MapOfKey32Property<Key32>> null_key32_keymap;
	NullImpl <MapOfKey32Property<Float32>> null_float32_keymap;
	NullImpl <MapOfKey32Property<Float64>> null_float64_keymap;
	NullImpl <MapOfKey32Property<Archive>> null_archive_keymap;
	NullImpl <MapOfKey32Property<CString>> null_cstring_keymap;
	NullImpl <MapOfKey32Property<WString>> null_wstring_keymap;

	ValueTypeHandler propertysheet_bool;
	ValueTypeHandler propertysheet_uint32;
	ValueTypeHandler propertysheet_uint64;
	ValueTypeHandler propertysheet_int32;
	ValueTypeHandler propertysheet_int64;
	ValueTypeHandler propertysheet_float32;
	ValueTypeHandler propertysheet_float64;
	ValueTypeHandler propertysheet_cstring;
	ValueTypeHandler propertysheet_wstring;

	PropertySetFormatImpl propertysetformat;

	BinaryFormat binaryformat;


#ifndef REFLEX_MINIMAL
	StandardPropertySheetFormatImpl propertysheetformat;

	JsonFormat json;
	RIFF riff;
	XML xml;
	Markup markup;

#endif
};

Library::Library()
	: null_binary(g_non_allocator)
	, null_propertysets(g_non_allocator)
	, null_uint32s(g_non_allocator)
	, null_uint64s(g_non_allocator)
	, null_int32s(g_non_allocator)
	, null_int64s(g_non_allocator)
	, null_float32s(g_non_allocator)
	, null_float64s(g_non_allocator)
	, null_keys(g_non_allocator)
	, null_cstrings(g_non_allocator)
	, null_wstrings(g_non_allocator)

	, null_nulltype_keymap(g_non_allocator)
	, null_bool_keymap(g_non_allocator)
	, null_uint32_keymap(g_non_allocator)
	, null_uint64_keymap(g_non_allocator)
	, null_int32_keymap(g_non_allocator)
	, null_int64_keymap(g_non_allocator)
	, null_key32_keymap(g_non_allocator)
	, null_float32_keymap(g_non_allocator)
	, null_float64_keymap(g_non_allocator)
	, null_archive_keymap(g_non_allocator)
	, null_cstring_keymap(g_non_allocator)
	, null_wstring_keymap(g_non_allocator)

	, propertysheet_bool(MakeValueType<bool>("bool"))
	, propertysheet_uint32(MakeValueType<UInt32>("UInt32"))
	, propertysheet_uint64(MakeValueType<UInt64>("UInt64"))
	, propertysheet_int32(MakeValueType<Int32>("Int32"))
	, propertysheet_int64(MakeValueType<Int64>("Int64"))
	, propertysheet_float32(MakeValueType<Float32>("Float32"))
	, propertysheet_float64(MakeValueType<Float64>("Float64"))
	, propertysheet_cstring(MakeValueType<CString>("CString"))
	, propertysheet_wstring(MakeValueType<WString>("WString"))
	, propertysetformat(ID32("@rb0"))
#ifndef REFLEX_MINIMAL
	,xml(Detail::g_standard_propertysheet_interface)
	,markup(Detail::g_standard_propertysheet_interface)
#endif
{
	RemoveConst(KeyMap::kDynamicTypeInfo).tname = "ObjectOf@Map@(Key32,CString)";
	RemoveConst(PropertySetArray::kDynamicTypeInfo).tname = "ObjectOf@Array@Reference@Data::PropertySet";
	RemoveConst(BinaryProperty::kDynamicTypeInfo).tname = "ObjectOf@Array@UInt8";
	RemoveConst(ArrayOfInt32Property::kDynamicTypeInfo).tname = "ObjectOf@Array@Int32";
	RemoveConst(ArrayOfInt64Property::kDynamicTypeInfo).tname = "ObjectOf@Array@Int64";
	RemoveConst(ArrayOfUInt32Property::kDynamicTypeInfo).tname = "ObjectOf@Array@UInt32";
	RemoveConst(ArrayOfUInt64Property::kDynamicTypeInfo).tname = "ObjectOf@Array@UInt64";
	RemoveConst(ArrayOfKey32Property::kDynamicTypeInfo).tname = "ObjectOf@Array@Key32";
	RemoveConst(ArrayOfFloat32Property::kDynamicTypeInfo).tname = "ObjectOf@Array@Float32";
	RemoveConst(ArrayOfFloat64Property::kDynamicTypeInfo).tname = "ObjectOf@Array@Float64";
	RemoveConst(ArrayOfCStringProperty::kDynamicTypeInfo).tname = "ObjectOf@Array@CString";

	//null_float32s.value.Allocate(1);

	//*null_float32s.value.GetData() = 0.0f;

	//null_int32s.value.Allocate(1);

	//*null_int32s.value.GetData() = 0;

	InitialiseStandardPropertySet(propertysetformat);
}

Reflex::Detail::Module g_module = { "Reflex::Data", Reflex::System::module };
Reflex::Detail::Module::Member <Library> g_library(g_module);

REFLEX_END_INTERNAL

const Reflex::Detail::Module & Reflex::Data::module = g_module;

REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::ArchiveObject, Reflex::Data::g_library->null_binary);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::PropertySetArray, Reflex::Data::g_library->null_propertysets);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::ArrayOfUInt32Property, Reflex::Data::g_library->null_uint32s);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::ArrayOfUInt64Property, Reflex::Data::g_library->null_uint64s);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::ArrayOfInt32Property, Reflex::Data::g_library->null_int32s);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::ArrayOfInt64Property, Reflex::Data::g_library->null_int64s);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::ArrayOfKey32Property, Reflex::Data::g_library->null_keys);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::ArrayOfFloat32Property, Reflex::Data::g_library->null_float32s);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::ArrayOfFloat64Property, Reflex::Data::g_library->null_float64s);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::ArrayOfCStringProperty, Reflex::Data::g_library->null_cstrings);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::ArrayOfWStringProperty, Reflex::Data::g_library->null_wstrings);

REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::MapOfKey32Property<Reflex::NullType>, Reflex::Data::g_library->null_nulltype_keymap);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::MapOfKey32Property<bool>, Reflex::Data::g_library->null_bool_keymap);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::MapOfKey32Property<Reflex::UInt32>, Reflex::Data::g_library->null_uint32_keymap);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::MapOfKey32Property<Reflex::UInt64>, Reflex::Data::g_library->null_uint64_keymap);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::MapOfKey32Property<Reflex::Int32>, Reflex::Data::g_library->null_int32_keymap);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::MapOfKey32Property<Reflex::Int64>, Reflex::Data::g_library->null_int64_keymap);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::MapOfKey32Property<Reflex::Key32>, Reflex::Data::g_library->null_key32_keymap);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::MapOfKey32Property<Reflex::Float32>, Reflex::Data::g_library->null_float32_keymap);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::MapOfKey32Property<Reflex::Float64>, Reflex::Data::g_library->null_float64_keymap);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::MapOfKey32Property<Reflex::Data::Archive>, Reflex::Data::g_library->null_archive_keymap);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::MapOfKey32Property<Reflex::CString>, Reflex::Data::g_library->null_cstring_keymap);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Data::MapOfKey32Property<Reflex::WString>, Reflex::Data::g_library->null_wstring_keymap);

Reflex::Data::PropertySet & Reflex::Data::PropertySet::null = Reflex::Data::g_library->null_propertyset;
Reflex::Data::Format & Reflex::Data::Format::null = Reflex::Data::g_library->null_format;
Reflex::Data::SerializableFormat & Reflex::Data::SerializableFormat::null = Reflex::Data::g_library->null_format;
Reflex::Data::iStreamable & Reflex::Data::iStreamable::null = Reflex::Data::g_library->null_streamable;

const Reflex::Data::ValueTypeHandler & Reflex::Data::kPropertySheetBool = g_library->propertysheet_bool;
const Reflex::Data::ValueTypeHandler & Reflex::Data::kPropertySheetUInt32 = g_library->propertysheet_uint32;
const Reflex::Data::ValueTypeHandler & Reflex::Data::kPropertySheetUInt64 = g_library->propertysheet_uint64;
const Reflex::Data::ValueTypeHandler & Reflex::Data::kPropertySheetInt32 = g_library->propertysheet_int32;
const Reflex::Data::ValueTypeHandler & Reflex::Data::kPropertySheetInt64 = g_library->propertysheet_int64;
const Reflex::Data::ValueTypeHandler & Reflex::Data::kPropertySheetFloat32 = g_library->propertysheet_float32;
const Reflex::Data::ValueTypeHandler & Reflex::Data::kPropertySheetFloat64 = g_library->propertysheet_float64;
const Reflex::Data::ValueTypeHandler & Reflex::Data::kPropertySheetCString = g_library->propertysheet_cstring;
const Reflex::Data::ValueTypeHandler & Reflex::Data::kPropertySheetWString = g_library->propertysheet_wstring;

const Reflex::ConstTRef <Reflex::Data::SerializableFormat> Reflex::Data::kBinaryFormat = g_library->binaryformat;
const Reflex::ConstTRef <Reflex::Data::SerializableFormat> Reflex::Data::kPropertySetFormat = Reflex::Data::g_library->propertysetformat;

#ifndef REFLEX_MINIMAL
const Reflex::ConstTRef <Reflex::Data::Detail::StandardPropertySheetInterface> Reflex::Data::Detail::g_standard_propertysheet_interface = Reflex::Data::g_library->propertysheetformat;

const Reflex::ConstTRef <Reflex::Data::Format> Reflex::Data::kPropertySheetFormat = Reflex::Data::g_library->propertysheetformat;
const Reflex::ConstTRef <Reflex::Data::Format> Reflex::Data::kJsonFormat = Reflex::Data::g_library->json;
const Reflex::ConstTRef <Reflex::Data::Format> Reflex::Data::kRiffFormat = Reflex::Data::g_library->riff;

const Reflex::ConstTRef <Reflex::Data::Format> Reflex::Data::kReflexXmlFormat = Reflex::Data::g_library->xml;
const Reflex::ConstTRef <Reflex::Data::Format> Reflex::Data::kReflexMarkupFormat = Reflex::Data::g_library->markup;
#endif

REFLEX_NOINLINE void Reflex::Data::Append(Data::Archive & archive, const Data::Archive::View & data)
{
	archive.Append(data);
}

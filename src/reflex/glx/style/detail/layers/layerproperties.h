#pragma once

#include "[require].h"
#include "types.h"




//
//declarations

#define GLX_DEFINE_LAYER_CLASS(CLASS, NAME) const Layer::Class CLASS::kClass(NAME, &CLASS::Create)
#define GLX_DEFINE_LAYER_CLASS_EX(CLASS, NAME, var) const Reflex::GLX::Detail::Layer::ClassEx var(NAME, &Reflex::GLX::Detail::CLASS::InitProperties, [](const Style & style, const Reflex::Object & schema, const Data::PropertySet & params) -> TRef <Layer> { return New<CLASS>(style, schema, params); })
#define GLX_DEFINE_LAYER_CLASS_X(CLASS, var) GLX_DEFINE_LAYER_CLASS_EX(CLASS,REFLEX_STRINGIFY(CLASS),var)

#define GLX_DECLARE_ENUM(NAME, ...) static constexpr Key32 st_##NAME[] = { __VA_ARGS__ }; inline static const ArrayView <Key32> k##NAME = { st_##NAME, GetArraySize(st_##NAME)};

REFLEX_NS(Reflex::GLX::Detail)

inline void SetLayerInitFn(GenericPropertiesSchema & schema, FunctionPointer <void(GenericLayer&, const void*, void*, GenericLayer::VTable&)> fn) { schema.oninit = reinterpret_cast<FunctionPointer <void(GenericLayer&, const void*, void*, void*)>>(fn); }

enum PropertyGroup : UInt8
{
	kPropertyGroupNone = 0,
	kPropertyGroupIndent = 1,
	//kPropertyGroupSize = 2,
	kPropertyGroupColour = 4,
	kPropertyGroupCustom0 = 8,
	kPropertyGroupCustom1 = 16,
	kPropertyGroupCustom2 = 32,
};

constexpr Float32 kMin = 0.00000001f;

struct StandardProperties
{
	Margin indent;
};

struct StandardPropertiesWithColour : public StandardProperties
{
	Colour colour = kWhite;
};

struct StandardPropertiesWithColourAndUnit : public StandardPropertiesWithColour
{
	Key32 unit;
};

struct StandardScratch
{
	Rect inner;
};

struct TrivialObjectData : public GenericLayer::ObjectState
{
	TrivialObjectData(GLX::Object & object, const GenericPropertiesSchema & schema, UInt16 propertyflags, const void * staticproperties)
		: GenericLayer::ObjectState(object, propertyflags)
	{
		MemCopy(staticproperties, state, schema.state_size);
	}
};

struct ComplexObjectData : public GenericLayer::ObjectState
{
	ComplexObjectData(GLX::Object & object, const GenericPropertiesSchema & schema, UInt16 propertyflags, const void * staticproperties);

	~ComplexObjectData();
};


void RegisterIndent(GenericPropertiesSchema & schema, UIntNative indent_offset = 0);

void RegisterIndentAndColour(GenericPropertiesSchema & schema, UIntNative indent_offset = 0);


void RegisterBoolProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup group = kPropertyGroupNone, PropertyBindStage bindable = kBindStageRealign);

void RegisterEnumProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, const ArrayView <Key32> * values, PropertyGroup group = kPropertyGroupNone, PropertyBindStage bindable = kBindStageNone);

void RegisterKey32Property(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup group = kPropertyGroupNone, PropertyBindStage bindable = kBindStageRealign);

void RegisterFloat32Property(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup group = kPropertyGroupNone, PropertyBindStage bindable = kBindStageRealign);

void RegisterFloatArrayProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup group = kPropertyGroupNone, PropertyBindStage bindable = kBindStageAccommodate);

void RegisterAngleProperty(GenericPropertiesSchema & schema, UIntNative offset, PropertyGroup group = kPropertyGroupNone, PropertyBindStage bindable = kBindStageRealign);

void RegisterAxisProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup group = kPropertyGroupNone, PropertyBindStage bindable = kBindStageRealign);

void RegisterRangeProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup group = kPropertyGroupNone, PropertyBindStage bindable = kBindStageRealign);

void RegisterColourProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup group = kPropertyGroupNone, PropertyBindStage bindable = kBindStageRealign);

void RegisterMarginProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup group = kPropertyGroupNone, PropertyBindStage bindable = kBindStageRealign);

void RegisterSizeProperty(GenericPropertiesSchema & schema,UIntNative offset,const CString::View & name,PropertyGroup group = kPropertyGroupNone,PropertyBindStage bindable = kBindStageRealign);

void RegisterPointProperty(GenericPropertiesSchema & schema,UIntNative offset,const CString::View & name,PropertyGroup group = kPropertyGroupNone,PropertyBindStage bindable = kBindStageRealign);

void RegisterAlignmentProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup group = kPropertyGroupNone, PropertyBindStage bindable = kBindStageRealign);


template <class TYPE> void RegisterReferenceProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup group = kPropertyGroupNone);

void RegisterLayersProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup group = kPropertyGroupNone, PropertyBindStage bindable = kBindStageNone);


extern const decltype(GenericLayer::VTable::OnAlign) * kStandardIndents;

inline decltype(GenericLayer::VTable::OnAlign) BindStandardAlign(GenericLayer & self)
{
	return kStandardIndents[self.propertyflags & 3];
}

inline void StandardIndent(const GenericLayer & layer, GenericLayer::ObjectState & self, Size size, Float & contenth)
{
	kStandardIndents[self.propertyflags & 3](layer, self, size, contenth);
}

template <bool INDENT> inline void StandardIndentImpl(const GenericLayer & layer, GenericLayer::ObjectState & self, Size size, Float & contenth)
{
	auto properties = self.GetProperties<StandardProperties>();

	auto scratch = self.GetScratch<StandardScratch>(layer);

	auto & inner = scratch->inner;

	if constexpr (INDENT)
	{
		inner = Indent(size, properties->indent);
	}
	else
	{
		inner = { kOrigin, size };
	}
}

void RegisterReferencePropertyImpl(GenericPropertiesSchema & schema, UIntNative offset, TypeID type_id, const CString::View & name, PropertyGroup  flags);

template <class TYPE> inline UInt64 MakeObjectOfData()
{
	typedef ObjectOf <TYPE> Object;

	return Reinterpret<UInt64>(MakeTuple(REFLEX_TYPEID(ObjectOf<TYPE>), UInt16(REFLEX_OFFSETOF(Object,value)), UInt16(sizeof(TYPE))));
}

void RegisterValue(GenericPropertiesSchema & schema, UIntNative offset, UInt64 objectofdata, const CString::View & name, PropertyGroup flags, PropertyBindStage bindable, GenericPropertiesSchema::Item::Setter from_numbers);

void NumbersToSize(const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags);

REFLEX_END

inline void Reflex::GLX::Detail::RegisterFloat32Property(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup flags, PropertyBindStage bindable)
{
	RegisterValue(schema, offset, MakeObjectOfData<Float>(), name, flags, bindable, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		if (auto & values = Cast<Data::ArrayOfFloat32Property>(object)->value)
		{
			*Cast<Float>(adr) = values.GetFirst();
		}
	});
}

inline void Reflex::GLX::Detail::RegisterSizeProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup flags, PropertyBindStage bindable)
{
	RegisterValue(schema, offset, MakeObjectOfData<Size>(), name, flags, bindable, &NumbersToSize);
}

inline void Reflex::GLX::Detail::RegisterPointProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup flags, PropertyBindStage bindable)
{
	RegisterValue(schema, offset, MakeObjectOfData<Point>(), name, flags, bindable, &NumbersToSize);
}

inline void Reflex::GLX::Detail::RegisterColourProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup flags, PropertyBindStage bindable)
{
	RegisterValue(schema, offset, MakeObjectOfData<Colour>(), name, flags, bindable, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		*Cast<Colour>(adr) = ToColour(Cast<Data::ArrayOfFloat32Property>(object)->value);
	});
}

inline void Reflex::GLX::Detail::RegisterMarginProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup flags, PropertyBindStage bindable)
{
	RegisterValue(schema, offset, MakeObjectOfData<Margin>(), name, flags, bindable, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		*Cast<Margin>(adr) = ToMargin(Cast<Data::ArrayOfFloat32Property>(object)->value, stylesheet_flags);
	});
}

template <class TYPE> inline void Reflex::GLX::Detail::RegisterReferenceProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup flags)
{
	RegisterReferencePropertyImpl(schema, offset, REFLEX_TYPEID(TYPE), name, flags);
}


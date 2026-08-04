#include "layerproperties.h"




//
//declarations

REFLEX_NS(Reflex::GLX::Detail)

typedef ObjectOf < Pair <Orientation> > OrientationObject;

REFLEX_TBINDER_1P(StandardIndentImpl);

constexpr UInt32 kAxisKeys[] = { K32(""), K32("x"), K32("y"), K32("xy"), K32("false"), K32("x"), K32("y"), ktrue };

constexpr Float32 kAlignmentToAngle[kNumAlignment] =
{
	0.125f,  // Top-left (45 degrees)
	0.25f,   // Top (90 degrees)
	0.375f,  // Top-right (135 degrees)

	1.0f,     // Left (360 degrees, equivalent to 0 degrees)
	0.0f,
	0.5f,    // Right (180 degrees)

	0.875f,  // Bottom-left (315 degrees)
	0.75f,   // Bottom (270 degrees)
	0.625f,  // Bottom-right (225 degrees)
};

void SetValue32(const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
{
	*Cast<UInt32>(adr) = Cast<Data::UInt32Property>(object)->value;
}

REFLEX_END

const Reflex::GLX::Alignment Reflex::GLX::Detail::kAlignmentExToAlignment[16] =
{
	kAlignmentTopLeft,		// Near, Near
	kAlignmentTop,			// Near, Center
	kAlignmentTopRight,		// Near, Far
	kAlignmentTop,	// Near, Fit

	kAlignmentLeft,			// Center, Near
	kAlignmentCenter,		// Center, Center
	kAlignmentRight,			// Center, Far
	kNumAlignment,	// Center, Fit

	kAlignmentBottomLeft,	// Far, Near
	kAlignmentBottom,		// Far, Center
	kAlignmentBottomRight,	// Far, Far
	kAlignmentBottom,	// Far, Fit

	kAlignmentLeft,       // Fit, Near
	kNumAlignment,       // Fit, Center
	kAlignmentRight,       // Fit, Far
	kNumAlignment        // Fit, Fit
};

const Reflex::GLX::Size Reflex::GLX::Detail::kAxisToSize[4] = { {}, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } };

const decltype(Reflex::GLX::Detail::GenericLayer::VTable::OnAlign) * Reflex::GLX::Detail::kStandardIndents = &REFLEX_TBIND(StandardIndentImpl, 0);

void Reflex::GLX::Detail::NumbersToSize(const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
{
	*Cast<Size>(adr) = ToSize(Cast<Data::ArrayOfFloat32Property>(object)->value);
}

void Reflex::GLX::Detail::RegisterBoolProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup flags, PropertyBindStage bindable)
{
	Key32 id = name;

	schema.RegisterProperty({ UInt16(offset), flags, bindable, name.data, MakeAddress<Data::BoolProperty>(id), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		*Cast<bool>(adr) = Cast<Data::BoolProperty>(object)->value;
	} });

	schema.RegisterProperty({ UInt16(offset), flags, bindable, name.data, MakeAddress<Data::Key32Property>(id), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		*Cast<bool>(adr) = Cast<Data::Key32Property>(object)->value == ktrue;
	} });
}

void Reflex::GLX::Detail::RegisterFloatArrayProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup flags, PropertyBindStage bindable)
{
	typedef Data::ArrayOfFloat32Property Type;

	schema.RegisterProperty({ UInt16(offset), flags, bindable, name.data, MakeAddress<Type>(name), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		*Cast<ConstReference<Type>>(adr) = Cast<Type>(object);
	} });

	RegisterReferenceProperty<Type>(schema, offset, name);
}

void Reflex::GLX::Detail::RegisterEnumProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, const ArrayView <Key32> * values, PropertyGroup group, PropertyBindStage bindable)
{
	schema.RegisterProperty({ .offset = UInt16(offset), .flags = group, .bindable = bindable, .tname = name.data, .address = MakeAddress<Data::Key32Property>(name), .data = ToUIntNative(values), .apply = [](const Reflex::Object & source_property, void * dest, UInt64 data, UInt16 stylesheet_flags)
	{
		*Cast<UInt8>(dest) = ParseEnum(Cast<Data::Key32Property>(source_property)->value, *ToPointer<ArrayView<Key32>>(data));
	} });
}

void Reflex::GLX::Detail::RegisterKey32Property(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup flags, PropertyBindStage bindable)
{
	Key32 id = name;

	schema.RegisterProperty({ UInt16(offset), flags, bindable, name.data, MakeAddress<Data::Key32Property>(id), 0, &SetValue32 });
}

void Reflex::GLX::Detail::RegisterAngleProperty(GenericPropertiesSchema & schema, UIntNative offset, PropertyGroup flags, PropertyBindStage bindable)
{
	constexpr auto id = K32("angle");

	CString::View name = "angle";

	RegisterFloat32Property(schema, offset, name, flags, bindable);

	schema.Pop();

	schema.RegisterProperty({ UInt16(offset), flags, bindable, name.data, MakeAddress<Data::Key32Property>(id), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		*Cast<Float>(adr) = kAlignmentToAngle[ParseEnum(Cast<Data::Key32Property>(object)->value, ToView(GLX::Detail::kAlignmentKeys))];
	} });
}

void Reflex::GLX::Detail::RegisterAxisProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup flags, PropertyBindStage bindable)
{
	Key32 id = name;

	schema.RegisterProperty({ UInt16(offset), flags, bindable, name.data, MakeAddress<Data::Key32Property>(id), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		*Reinterpret<UInt8>(adr) = ParseEnum(Cast<Data::Key32Property>(object)->value, Reinterpret<ArrayView<Key32>>(ToView(kAxisKeys)), 3) & 3;
	} });
}

void Reflex::GLX::Detail::RegisterRangeProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup flags, PropertyBindStage bindable)
{
	Key32 id = name;

	auto pdefs = schema.RegisterProperties(3);

	//pdefs[0] = { UInt16(offset), flags, bindable, "{id}_origin", MakeAddress<Data::Float32Property>(Join(name, "_origin")), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	//{
	//	auto & range = *Cast<Range>(adr);

	//	range.a = Cast<Data::Float32Property>(object)->value;
	//} };

	pdefs[0] = { UInt16(offset), flags, bindable, name.data, MakeAddress<RangeProperty>(id), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		auto & range = *Cast<Range>(adr);

		range = Cast<RangeProperty>(object)->value;
	} };

	pdefs[1] = { UInt16(offset), flags, bindable, name.data, MakeAddress<Data::Float32Property>(id), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		auto & range = *Cast<Range>(adr);

		range.length = Cast<Data::Float32Property>(object)->value;
	} };

	pdefs[2] = { UInt16(offset), flags, kBindStageNone, name.data, MakeAddress<Data::ArrayOfFloat32Property>(id), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		auto & range = *Cast<Range>(adr);

		auto & values = Cast<Data::ArrayOfFloat32Property>(object)->value;

		switch (values.GetSize())
		{
		case 2:
			range = { values[0], values[1] };
			break;

		case 1:
			range = { 0.0f, values[0] };
			break;
		}
	} };
}

void Reflex::GLX::Detail::RegisterValue(GenericPropertiesSchema & schema, UIntNative offset, UInt64 object_data, const CString::View & name, PropertyGroup flags, PropertyBindStage bindable, GenericPropertiesSchema::Item::Setter from_numbers)
{
	Key32 id = name;

	auto defs = schema.RegisterProperties(3);

	Address adr = { id, Reinterpret < Tuple<TypeID, UInt16, UInt16>>(object_data).a };

	switch (Reinterpret < Tuple<TypeID, UInt16, UInt16>>(object_data).c)
	{
	case 4:
		defs[0] = { UInt16(offset), flags, bindable, name.data, adr, 0, &SetValue32 };
		break;

	default:
		defs[0] = { UInt16(offset), flags, bindable, name.data, adr, object_data, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
		{
			auto & tuple = Reinterpret<Tuple<UInt32,UInt16,UInt16>>(data);

			MemCopy(Reinterpret<UInt8>(&object) + tuple.b, adr, tuple.c);
		} };
		break;
	}

	defs[1] = { UInt16(offset), flags, kBindStageNone, name.data, MakeAddress<Data::ArrayOfFloat32Property>(id), object_data, from_numbers };

	defs[2] = { UInt16(offset), flags, kBindStageNone, name.data, MakeAddress<Data::Key32Property>(id), object_data, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		auto id = Cast<Data::Key32Property>(object)->value;

		if (auto p = FindResource(st_current_style, Address{ id, Reinterpret<TypeID>(data) }, 0))
		{
			auto & tuple = Reinterpret<Tuple<UInt32, UInt16, UInt16>>(data);

			MemCopy(Reinterpret<UInt8>(p) + tuple.b, adr, tuple.c);
		}
	} };
}

void Reflex::GLX::Detail::RegisterAlignmentProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup flags, PropertyBindStage bindable)
{
	Key32 id = name;

	auto pdefs = schema.RegisterProperties(3);

	pdefs[0] = { UInt16(offset), flags, bindable, name.data, MakeAddress<OrientationObject>(id), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		*Cast< Pair <Orientation> >(adr) = Cast<OrientationObject>(object)->value;
	} };

	pdefs[1] = { UInt16(offset), flags, kBindStageNone, name.data, MakeAddress<Data::Key32Property>(id), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		auto key = Cast<Data::Key32Property>(object)->value;

		auto alignment = ParseAlignment(key);

		*Cast< Pair <Orientation> >(adr) = kAlignmentToOrientation[alignment];
	} };

	pdefs[2] = { UInt16(offset), flags, kBindStageNone, name.data, MakeAddress<Data::ArrayOfKey32Property>(id), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		auto & keys = Cast<Data::ArrayOfKey32Property>(object)->value;

		if (keys.GetSize() == 2)
		{
			auto ptr = Cast<Orientation>(adr);

			REFLEX_LOOP(idx, 2) ptr[idx] = Orientation(ParseEnum(keys[idx], ToView(kOrientationKeys)));
		}
	} };
}

void Reflex::GLX::Detail::RegisterLayersProperty(GenericPropertiesSchema & schema, UIntNative offset, const CString::View & name, PropertyGroup flags, PropertyBindStage bindable)
{
	Key32 id = name;

	auto pdefs = schema.RegisterProperties(3);

	pdefs[0] = { UInt16(offset), flags, kBindStageNone, name.data, MakeAddress<LayerDesc>(id), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		auto precompiled = Cast<LayerDesc>(object);

		auto layer = Detail::Layer::Create(precompiled->cls, st_current_style, precompiled);

		Cast<ArrayOfLayer>(adr)->Push(layer);
	} };

	pdefs[1] = { UInt16(offset), flags, kBindStageNone, name.data, MakeAddress<ArrayOfLayerDesc>(id), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		Detail::CreateLayers(st_current_style, Cast<ArrayOfLayerDesc>(object), *Cast<ArrayOfLayer>(adr));
	} };

	pdefs[2] = { UInt16(offset), flags, kBindStageNone, name.data, MakeAddress<Data::Key32Property>(id), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		FindAndSetLayer(*Cast<ArrayOfLayer>(adr), Cast<Data::Key32Property>(object));
	} };
}

void Reflex::GLX::Detail::RegisterIndent(GenericPropertiesSchema & schema, UIntNative indent_offset)
{
	UInt16 indent = UInt16(indent_offset);

	RegisterMarginProperty(schema, indent, "indent", kPropertyGroupIndent);

	auto pdefs = schema.RegisterProperties(1);

	pdefs[0] = { indent, kPropertyGroupIndent, kBindStageNone, "offset", MakeAddress<Data::ArrayOfFloat32Property>(K32("offset")), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		auto & indent = *Cast<Margin>(adr);

		auto offset = Reinterpret<Point>(ToSize(Cast<Data::ArrayOfFloat32Property>(object)->value));

		indent.near.w = offset.x;

		indent.near.h = offset.y;

		indent.far.w = -offset.x;

		indent.far.h = -offset.y;
	} };
}

void Reflex::GLX::Detail::RegisterIndentAndColour(GenericPropertiesSchema & schema, UIntNative indent_offset)
{
	RegisterIndent(schema, indent_offset);

	RegisterColourProperty(schema, indent_offset + REFLEX_OFFSETOF(StandardPropertiesWithColour, colour), "color", kPropertyGroupColour, kBindStageRedraw);
	RegisterColourProperty(schema, indent_offset + REFLEX_OFFSETOF(StandardPropertiesWithColour, colour), "colour", kPropertyGroupColour, kBindStageRedraw);
}

void Reflex::GLX::Detail::RegisterReferencePropertyImpl(GenericPropertiesSchema & schema, UIntNative offset, TypeID type_id, const CString::View & name, PropertyGroup flags)
{
	Key32 id = name;

	schema.RegisterProperty({ UInt16(offset), flags, kBindStageNone, name.data, MakeAddress<Data::Key32Property>(id), type_id, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		auto id = Cast<Data::Key32Property>(object)->value;

		if (auto p = FindResource(st_current_style, Address{ id, Reinterpret<TypeID>(data) }, 0))
		{
			*Cast<ConstReference<Reflex::Object>>(adr) = p;
		}
	} });
}


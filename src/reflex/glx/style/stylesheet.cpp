#include "stylesheet.h"

#include "detail/cstyle.h"

#include "reflex/glx/detail/vector.h"
#include "reflex/glx/detail/functions.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

struct StyleSheetDecoder
{
	REFLEX_INLINE StyleSheetDecoder(const WString::View & path, StyleSheet & sheet, const Data::Archive::View & text, const Data::PropertySet & options)
		: prev(g_current_stylesheet)
	{
#if (REFLEX_DEBUG)
		auto pre_decoded = Data::DecodePropertySet(Data::kPropertySheetFormat, text);

		Data::Assimilate(g_library->keymap, Data::GetKeyMap(pre_decoded));
#endif
		g_current_stylesheet = { File::SplitFilename(path).a, &sheet, &options };

		auto library = g_library;

		Data::UnsetPropertySet(sheet, Data::kError);

		TRef format = library->kStyleSheetFormat;

		format->Reset(sheet);

		format->Decode(sheet, text);

#if (REFLEX_DEBUG)
		auto [line, stage, desc] = Data::GetError(sheet);

		if (line != kMaxUInt32)
		{
			output.LogEx(kLogError, {}, '[',path, ':', line ,']', stage, ' ', desc);
		}
#endif
	}

	REFLEX_INLINE ~StyleSheetDecoder()
	{
		g_current_stylesheet = prev;
	}

	StyleSheetContext prev;
};

REFLEX_INLINE void ValidateType(const CString::View & type, Data::Detail::PropertySheetInterface::TokenType token_t, const CString::View & value)
{
#if REFLEX_DEBUG
	if (type)
	{
		switch (MakeKey32(type))
		{
		case K32("Float32"):
		case K32("Int32"):
		case K32("Size"):
		case K32("Margin"):
		case K32("Colour"):
			return;

		default:
			ThrowStylesheetParseError(kInvalidType, type);
			break;
		}
	}
#endif
}

REFLEX_END_INTERNAL

Reflex::GLX::StyleSheetContext Reflex::GLX::g_current_stylesheet;

Reflex::TRef <Reflex::Object> Reflex::GLX::Detail::DecodeStyleSheet(const File::ResourcePool::StreamContext & ctx, System::FileHandle & instream)
{
	Key32 id = ctx.path;

	auto self = REFLEX_CREATE(StyleSheet, id, Data::GetFloat32(ctx.options, K32("scale"), 1.0f));

	RemoveConst(self->path) = id;	//!needed, ctr sets id from path, which can be overridden by sheet property

	REFLEX_IF_DEBUG(Data::RegisterKey(g_library->keymap, ToCString(ctx.path)));

	Reflex::Detail::SilentReference <StyleSheet> retain(self);

	StyleSheetDecoder decode(ctx.path, *self, File::ReadBytes(instream), ctx.options);

	return self;
}

Reflex::Data::Detail::PropertySheetInterface & Reflex::GLX::StyleSheetParser::GetInterface(ObjectWithType & object)
{
	if (object.b == REFLEX_TYPEID(Style))
	{
		return *this;
	}
	else
	{
		return Detail::iresource;
	}
}

bool Reflex::GLX::StyleSheetParser::Begin(Data::PropertySet & root) const
{
	return DynamicCast<Style>(root);
}

bool Reflex::GLX::StyleSheetParser::OnSetOption(Key32 id, const CString::View & value) const
{
	if (id == "margin_syntax")
	{
		switch (MakeKey32(value))
		{
		case K32("css"):
			RemoveConst(g_current_stylesheet.sheet->stylesheet_flags) |= 1;
			return true;

		default:
			return false;
		}
	}

	return false;
}

bool Reflex::GLX::ResourceParser::OnCondition(const ArrayView < Pair <TokenType, CString::View> > & expression) const
{
	constexpr auto IsNumber = [](TokenType type)
	{
		return type == kTokenTypeFloat || type == kTokenTypeInt;
	};

	auto pcondition = expression.data;

	if (expression.size == 1)
	{
		return Data::GetBool(*g_current_stylesheet.options, pcondition->b);
	}
	else if (expression.size > 2 && IsNumber(expression.GetLast().a))
	{
		auto value = Data::GetFloat32(*g_current_stylesheet.options, pcondition->b);

		auto last = expression.GetLast().b;

		auto compare = ToFloat32(last);

		auto op_start = expression[1].b.data;

		CString::View op = { op_start, UInt(last.data - op_start) };

		switch (MakeKey32(Trim(op)))
		{
		case K32("<"):
			return value < compare;

		case K32("<="):
			return value >= compare;

		case K32(">"):
			return value > compare;

		case K32(">="):
			return value >= compare;
		}
	}

	ThrowStylesheetParseError({}, "invalid expression");

	return false;
}

Reflex::Data::Detail::PropertySheetInterface::ObjectWithType Reflex::GLX::ResourceParser::CreateValue(Data::KeyMap & keymap, const CString::View & type_string, TokenType value_t, const CString::View & value) const
{
	ValidateType(type_string, value_t, value);

	Key32 type = type_string;

	switch (value_t)
	{
	case kTokenTypeWord:
	case kTokenTypeSingleQuotedString:
		return Data::Detail::CreateObjectWithType<Data::Key32Property>(value);

	case kTokenTypeBool:
		return Data::Detail::CreateObjectWithType<Data::Key32Property>(value);

	case kTokenTypeDoubleQuotedString:
		return Data::Detail::CreateObjectWithType<Data::CStringProperty>(value);

	case kTokenTypeInt:
	case kTokenTypeFloat:
	{
		Float32 fvalue = ToFloat32(value);
		Float32 luminance;

		switch (MakeKey32(type_string))
		{
		case K32("Float32"):
			return Data::Detail::CreateObjectWithType<Data::Float32Property>(fvalue);

		case K32("Int32"):
			return Data::Detail::CreateObjectWithType<Data::Int32Property>(ToInt32(value));

		case K32("Colour"):
			luminance = fvalue / 255.0f;
			return Data::Detail::CreateObjectWithType<ColourProperty>(luminance, luminance, luminance, 1.0f);

		case K32("Size"):
			return Data::Detail::CreateObjectWithType<SizeProperty>(MakeSize(fvalue));

		case K32("Margin"):
			return Data::Detail::CreateObjectWithType<MarginProperty>(MakeMargin(fvalue));

		default:
			return Data::Detail::CreateObjectWithType<Data::ArrayOfFloat32Property>(std::initializer_list<Float32>({ fvalue }));
		}
	}

	case kTokenTypeReference:
		return Data::Detail::CreateObjectWithType<Detail::ReferenceProperty>(value);

	case kTokenTypeHex:
		return Data::Detail::CreateObjectWithType<ColourProperty>(Detail::ToColour(value));

	default:
		return {};
	}
}

Reflex::Data::Detail::PropertySheetInterface::ObjectWithType Reflex::GLX::ResourceParser::CreateValueArray(Data::KeyMap & keymap, const CString::View & type, TokenType tokentype, const Array <CString::View> & values) const
{
	constexpr auto ToFloats = [](const Array <CString::View> &values) -> ArrayView <Float>
	{
		auto & cache = g_library->cache.int_workspace;

		cache.SetSize(values.GetSize());

		auto ptr = Reinterpret<Float>(cache.GetData());

		auto pdest = ptr;

		for (auto & i : values) (*pdest++) = ToFloat32(i);

		return { ptr, values.GetSize() };
	};

	switch (tokentype)
	{
	case kTokenTypeWord:
	case kTokenTypeSingleQuotedString:
	case kTokenTypeBool:
	{
		auto & to = *REFLEX_CREATE(Data::ArrayOfKey32Property, values.GetSize());

		auto pdest = to.value.GetData();

		for (auto & i : values) (*pdest++) = i;

		return Data::Detail::MakeObjectWithType(to);
	}

	case kTokenTypeFloat:
	case kTokenTypeInt:
	{
		switch (MakeKey32(type))
		{
		case K32("Colour"):
			return Data::Detail::CreateObjectWithType<ColourProperty>(Detail::ToColour(ToFloats(values)));

		case K32("Margin"):
			return Data::Detail::CreateObjectWithType<MarginProperty>(Detail::ToMargin(ToFloats(values), g_current_stylesheet.sheet->stylesheet_flags));

		case K32("Size"):
			return Data::Detail::CreateObjectWithType<SizeProperty>(Detail::ToSize(ToFloats(values)));

		default:
			auto & to = *REFLEX_CREATE(Data::ArrayOfFloat32Property, values.GetSize());

			auto pdest = to.value.GetData();

			for (auto & i : values) (*pdest++) = ToFloat32(i);

			return Data::Detail::MakeObjectWithType(to);
		}
	}

	case kTokenTypeDoubleQuotedString:
	{
		auto & to = *REFLEX_CREATE(Data::ArrayOfCStringProperty, values.GetSize());

		REFLEX_LOOP(idx, values.GetSize()) to.value[idx] = values[idx];

		return Data::Detail::MakeObjectWithType(to);
	}

	default:
		break;
	};

	return {};
}

Reflex::Data::Detail::PropertySheetInterface::ObjectWithType Reflex::GLX::ResourceParser::CreateObject(Reflex::Object & parent, const CString::View & type_, Key32 id, bool is_stub) const
{
	auto type = MakeKey32(type_);

	if (auto pcls = Detail::Layer::Class::Query(type))
	{
		return Data::Detail::CreateObjectWithType<Detail::LayerDesc>(*pcls);
	}
	else if (auto parent_style = DynamicCast<Style>(parent))
	{
		switch (type)
		{
		case K32(""):
		case K32("Style"):
		case K32("State"):
		case K32("Alias"):
		{
			auto child = RemoveConst(Detail::GetChildStyle(*parent_style, id));
			if (!child)
			{
				child = parent_style->AddSubStyle(type, id, is_stub).Adr();
			}
			return Data::Detail::MakeObjectWithType(*child);
		}

		case K32("Font"):
		case K32("Bitmap"):
		case K32("VectorSet"):
		case K32("SVG"):
			return Data::Detail::CreateObjectWithType<Detail::ResourceDesc>(type);

		case K32("Data"):
			return Data::Detail::CreateObjectWithType<Data::PropertySet>();

		default:
			#if REFLEX_DEBUG
			if (type_.GetFirst() == '_')
			{
				if (!Detail::Layer::Class::Query(Nudge(type_)))
				{
					ThrowStylesheetParseError(kInvalidType, type_);
				}
			}
			#endif
			break;	//to NullLayer
		}
	}
	else if (parent.object_t == Detail::ResourceDesc::kDynamicTypeInfo)
	{
		return Data::Detail::CreateObjectWithType<Data::PropertySet>();
	}

	return Data::Detail::CreateObjectWithType<Detail::LayerDesc>(Detail::Layer::Class::null);
}

Reflex::Data::Detail::PropertySheetInterface::ObjectWithType Reflex::GLX::ResourceParser::CreateObjectArray(const CString::View & type, const Array <ObjectWithType> & objects) const
{
	if (objects)
	{
		auto type_id = g_layer_desc_typeid;

		if (objects.GetFirst().b == type_id)
		{
			auto rtn = New<Detail::ArrayOfLayerDesc>();

			rtn->value.Allocate(objects.GetSize());

			for (auto & i : objects)
			{
				if (i.b == type_id)
				{
					rtn->value.Push<kAllocateNone>(Cast<Detail::LayerDesc>(i.a));
				}
				else
				{
					ThrowStylesheetParseError(kInvalidType, type);
				}
			}

			return Data::Detail::MakeObjectWithType(*rtn);
		}
	}

	return Data::Detail::g_standard_propertysheet_interface->CreateObjectArray(type, objects);
}

Reflex::GLX::StyleSheet::StyleSheet(Key32 id, Float32 scale)
	: Style(id, 0)
	, kScale(scale)
{
}

const Reflex::GLX::Style * Reflex::GLX::StyleSheet::QuerySubStyle(Key32 key, const Style * fallback) const
{
	if (auto child = Detail::GetChildStyle(*this, key))
	{
		return child;
	}
	else
	{
		for (auto & i : m_includes_imports[0])
		{
			if (auto child = i->QuerySubStyle(key, 0))
			{
				return child;
			}
		}
	}

	return fallback;
}

REFLEX_NOINLINE Reflex::ConstTRef <Reflex::GLX::StyleSheet> Reflex::GLX::StyleSheet::AddInclude(const CString & i, bool import)
{
	auto include = Detail::RetrieveRelativeResource<StyleSheet>(ToWString(i), *g_current_stylesheet.options, &Detail::DecodeStyleSheet);

	if (IsValid(Cast<Reflex::Object>(include)))	//can be null object if accidentally includign self
	{
		RemoveConst(stylesheet_flags) |= include->stylesheet_flags;

		m_includes_imports[import].Push(include);
	}

	return include;
}

void Reflex::GLX::StyleSheet::OnSetProperty(Address address, Reflex::Object & object)
{
	switch (address.id.value)
	{
	case K32("include"):
		if (address.type_id == g_array_cstring_property_typeid)
		{
			for (auto & i : Cast<Data::ArrayOfCStringProperty>(object)->value)
			{
				AddInclude(i, false);
			}
		}
		else if (address.type_id == g_cstring_property_typeid)
		{
			AddInclude(Cast<Data::CStringProperty>(object)->value, false);
		}
		break;	//to AutoRelease

	case kid:
		if (address.type_id == g_key32_property_typeid)
		{
			RemoveConst(Style::id) = Cast<Data::Key32Property>(object)->value;
		}
		break;	//to AutoRelease

	default:
		Style::OnSetProperty(address, object);
		return;	//!important dont AutoRelease
	}

	AutoRelease(object);
}

Reflex::WString Reflex::GLX::GetPathProperty(const Data::PropertySet & propertyset)
{
	REFLEX_DECLARE_KEY32(path);

	return ToWString(GetProperty<Data::CStringProperty>(propertyset, kpath)->value);
}

Reflex::ConstTRef <Reflex::Object> Reflex::GLX::Detail::RetrieveRelativeResource(const WString::View & filename, const Data::PropertySet & options, TypeID type_id, File::ResourcePool::Ctr ctr)
{
	auto path = File::ResolveIncludePath(g_current_stylesheet.path, filename);

	File::ResourcePool::Lock lock(Core::desktop->resourcepool);

	return lock.RetrieveToken(type_id, path, options, ctr).object;
}

#if (REFLEX_DEBUG)
void Reflex::GLX::Detail::LayerDesc::OnSetProperty(Address adr, Object & object)
{
	if (auto schema = DynamicCast<GenericPropertiesSchema>(cls->schema))
	{
		bool has_id = false;

		if (adr.type_id == REFLEX_TYPEID(ReferenceProperty))
		{
			for (auto & i : schema->bindable)
			{
				if (i->address.id == adr.id)
				{
					ResourceDesc::OnSetProperty(adr, object);
					
					return;
				}
			}
		}
		else
		{
			for (auto & i : schema->defs)
			{
				if (i.address == adr)
				{
					ResourceDesc::OnSetProperty(adr, object);
					
					return;
				}
				else if (i.address.id == adr.id)
				{
					has_id = true;
				}
			}
		}

		ThrowInvalidPropertyError(Layer::Class::Query(schema->uid)->id, adr.id, object, has_id);
	}

	ResourceDesc::OnSetProperty(adr, object);
}
#endif

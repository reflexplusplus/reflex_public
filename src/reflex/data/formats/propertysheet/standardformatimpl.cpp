#include "standardformatimpl.h"
#include "../defines.h"




REFLEX_BEGIN_INTERNAL(Reflex::Data)

REFLEX_NOINLINE void WriteKey(const KeyMap & keymap, Archive & stream, const CString & indent, const Detail::StandardPropertySheetInterface::TypeInfo & type, bool implicit, Key32 key)
{
	CString key_string = GetKey(keymap, key);

	if (key_string)
	{
		auto number_type = Detail::CharType(kMaxUInt8);

		for (auto c : key_string)
		{
			auto type = Detail::kChar2Type[c];

			if (!(type == Detail::kCharTypeWord || type == number_type))
			{
				key_string = Join(char(39), key_string, char(39));

				break;
			}

			number_type = Detail::kCharTypeNumber;
		}
	}
	else
	{
		key_string = ToCString(key.value);
	}

	if (implicit)
	{
		Write(stream, Join(indent, key_string, ':'));
	}
	else
	{
		Write(stream, Join(indent, '@', type.type_name, ' ', key_string, ':'));
	}
}

UInt64 MakeTypeHandlerID(Key32 type, Detail::PropertySheetInterface::TokenType tokentype)
{
	UInt64 rtn = 0;
	
	Reinterpret<ValueHandlerID>(rtn).a = tokentype;
	
	Reinterpret<ValueHandlerID>(rtn).b = type;	//upper to allow filtering of implicits

	return rtn;
}

REFLEX_END_INTERNAL

Reflex::Data::StandardPropertySheetFormatImpl::StandardPropertySheetFormatImpl()
	: PropertySheetFormatImpl(*Cast<Detail::PropertySheetInterface>(this))
{
	//remember ORDER IS CRITICAL especially for Key32
	//the first one should be the implicit type (although this isnt critical)
	//the first or second one MUST be fallback
	//exporter relies on this order

	auto key32_t = MakeValueType<Key32>("Key32");

	auto key32_from_hex = key32_t;

	key32_from_hex.object_ctr = [](KeyMap &, const CString::View & value)
	{
		auto t = Data::HexToBytes(value);
		
		while (t.GetSize() < 4) t.Push('0');

		return Detail::CreateObjectWithType<Key32Property>(Data::Unpack<UInt32>(t));
	};

	key32_from_hex.array_ctr = [](KeyMap &, const Array <CString::View> & values)
	{
		auto rtn = REFLEX_CREATE(ArrayOfKey32Property, );

		rtn->value.SetSize(values.GetSize());

		auto prtn = rtn->value.GetData();

		REFLEX_LOOP_PTR(values.GetData(), pvalue, values.GetSize())
		{
			auto t = Data::HexToBytes(*pvalue);

			while (t.GetSize() < 4) t.Push('0');

			*prtn++ = Data::Unpack<UInt32>(t);
		}

		return Detail::MakeObjectWithType(*rtn);
	};

	key32_from_hex.to_string[0] = [](const KeyMap & keymap, const Object & object)
	{
		CString output;
		
		output.Push('$');

		output.Append(Data::BytesToHex(Data::Pack(Cast<Key32Property>(object)->value)));

		return output;
	};

	RegisterValueTypeHandler(K32("Key32"), kTokenTypeHex, key32_from_hex);

	RegisterImplicitValueType(K32("Key32"), kTokenTypeSingleQuotedString, key32_t);
	
	RegisterImplicitValueType(K32("Key32"), kTokenTypeWord, key32_t);


	RegisterImplicitValueType(K32("Float32"), kTokenTypeFloat, kPropertySheetFloat32);

	RegisterValueTypeHandler(K32("Float32"), kTokenTypeInt, kPropertySheetFloat32);


	RegisterImplicitValueType(K32("Int32"), kTokenTypeInt, kPropertySheetInt32);

	RegisterImplicitValueType(K32("String"), kTokenTypeDoubleQuotedString, kPropertySheetCString);
	
	RegisterImplicitValueType(K32("Binary"), kTokenTypeHex, MakeValueType<Archive>("Binary"));

	RegisterImplicitValueType(K32("bool"), kTokenTypeBool, kPropertySheetBool);


	
	//verbose types

	RegisterValueTypeHandler(K32("UInt32"), kTokenTypeInt, kPropertySheetUInt32);
	
	RegisterValueTypeHandler(K32("UInt64"), kTokenTypeInt, kPropertySheetUInt64);

	RegisterValueTypeHandler(K32("Int64"), kTokenTypeInt, kPropertySheetInt64);

	RegisterValueTypeHandler(K32("Float64"), kTokenTypeFloat, kPropertySheetFloat64);

	RegisterValueTypeHandler(K32("WString"), kTokenTypeDoubleQuotedString, kPropertySheetWString);



	//MAP

	RegisterObjectTypeHandler(kNullKey, MakePropertySetType<PropertySet>("PropertySet"));

}

void Reflex::Data::StandardPropertySheetFormatImpl::RegisterObjectTypeHandler(Key32 type_name, const PropertySetType & type)
{
	typedef Reflex::Detail::Initialiser <PropertySetType> Initialiser;

	REFLEX_STATIC_ASSERT(sizeof(Sequence<Key32, Initialiser>) == sizeof(m_map_types));

	Reinterpret<Sequence<Key32,Initialiser>>(m_map_types).Set(type_name).Init(type);

	for (auto & i : type.type_ids) PropertySheetFormatImpl::RegisterType(i);
}

void Reflex::Data::StandardPropertySheetFormatImpl::RegisterValueTypeHandler(Key32 type_name, TokenType tokentype, const ValueTypeHandler & type)
{
	typedef Reflex::Detail::Initialiser <ValueTypeHandler> Initialiser;

	REFLEX_STATIC_ASSERT(sizeof(Sequence<UInt64,Initialiser>) == sizeof(m_value_handlers));

	auto handlerid = MakeTypeHandlerID(type_name, tokentype);

	REFLEX_ASSERT(!m_value_handlers.Search(handlerid));

	Reinterpret<Sequence<UInt64,Initialiser>>(m_value_handlers).Set(handlerid).Init(type);

	for (auto & i : type.type_ids) PropertySheetFormatImpl::RegisterType(i);
}

void Reflex::Data::StandardPropertySheetFormatImpl::RegisterImplicitValueType(Key32 type_name, TokenType tokentype, const ValueTypeHandler & vtype)
{
	RegisterValueTypeHandler(kNullKey, tokentype, vtype);

	RegisterValueTypeHandler(type_name, tokentype, vtype);
}

Reflex::Data::Detail::PropertySheetInterface::ObjectWithType Reflex::Data::StandardPropertySheetFormatImpl::CreateValue(KeyMap & keymap, const CString::View & type_name, TokenType tokentype, const CString::View & value) const
{
	if (auto pscalar = m_value_handlers.SearchValue(MakeTypeHandlerID(type_name, tokentype)))
	{
		return pscalar->object_ctr(keymap, value);
	}

	return {};
}

Reflex::Data::Detail::PropertySheetInterface::ObjectWithType Reflex::Data::StandardPropertySheetFormatImpl::CreateValueArray(KeyMap & keymap, const CString::View & type_name, TokenType tokentype, const Array <CString::View> & values) const
{
	if (auto pscalar = m_value_handlers.SearchValue(MakeTypeHandlerID(type_name, tokentype)))
	{
		return pscalar->array_ctr(keymap, values);
	}

	return {};
}

Reflex::Data::Detail::PropertySheetInterface::ObjectWithType Reflex::Data::StandardPropertySheetFormatImpl::CreateObject(Object & parent, const CString::View & type, Key32 id, bool is_stub) const
{
	return m_map_types.SearchValue(type, &m_map_types.GetFirst().value)->object_ctr(parent, id);
}

Reflex::Data::Detail::PropertySheetInterface::ObjectWithType Reflex::Data::StandardPropertySheetFormatImpl::CreateObjectArray(const CString::View & type, const Array <ObjectWithType> & objects) const
{
	auto CreateGeneric = [](const Array <ObjectWithType> & objects)
	{
		auto rtn = REFLEX_CREATE(GenericObjectArray);

		rtn->value.Allocate(objects.GetSize());

		for (auto & i : objects) rtn->value.Push(i.a);

		return Detail::MakeObjectWithType(*rtn);
	};

	if (objects)
	{
		auto type_id = objects.GetFirst().b;

		for (auto & i : objects)
		{
			if (SetFiltered(type_id, i.b))
			{
				return CreateGeneric(objects);
			}
		}

		//uni variate

		for (auto & i : m_map_types)
		{
			auto & map_t = i.value;

			if (map_t.type_ids[0] == type_id)
			{
				return map_t.array_ctr(objects);
			}
		}

		return CreateGeneric(objects);
	}
	else
	{
		Key32 type_key = type;

		for (auto & i : m_value_handlers)
		{
			if (Reinterpret<ValueHandlerID>(i.key).b == type_key)
			{
				return i.value.array_ctr(Null<KeyMap>(), {});
			}
		}

		return m_map_types.SearchValue(type, &m_map_types.GetFirst().value)->array_ctr(objects);
	}
}

bool Reflex::Data::StandardPropertySheetFormatImpl::Export(const PropertySet & cpropertyset, ExportState & state, Archive & archive) const
{
	struct ValueExporter
	{
		ValueTypeHandler * implicit_explicit[2];
		bool isarray;
	};

	constexpr auto FindValueHandler = [](const StandardPropertySheetFormatImpl & self, TypeID type_id) -> ValueExporter
	{
		ValueTypeHandler * implicit = nullptr;
		ValueTypeHandler * fallback = nullptr;

		UInt isarray = false;

		auto implicit_start = MakeTypeHandlerID(kNullKey, kTokenTypeWord);

		decltype(self.m_value_handlers)::Range implicit_range(self.m_value_handlers, implicit_start, implicit_start + kMaxUInt32);

		REFLEX_RFOREACH(i, implicit_range)
	{
			if (auto idx = Search(i.value.type_ids, type_id))
			{
				implicit = &i.value;

				isarray = idx.value;

				break;
			}
		}

		for (auto & i : self.m_value_handlers)
		{
			auto & handlerid = Reinterpret<ValueHandlerID>(i.key);

			if (handlerid.b == kNullKey) continue;

			if (auto idx = Search(i.value.type_ids, type_id))
			{
				fallback = &i.value;

				isarray = idx.value;

				if (handlerid.a == kTokenTypeHex) break;	//special handling for Key32
			}
		}

		return { implicit, fallback, True(isarray) };
	};


	auto & propertyset = RemoveConst(cpropertyset);

	auto & keymap = state.keymap;

	auto & indent = state.indent;

	CString buffer;


	bool written = false;

	ValueExporter exporter = { 0,0,false };

	REFLEX_FOREACH(itr, propertyset.Iterate())
	{
		UInt32 type_id = 0;

		if (SetFiltered(type_id, itr.key.type_id))
		{
			exporter = FindValueHandler(*this, type_id);
		}

		//auto size = archive.GetSize();

		REFLEX_LOOP(idx, 2)
		{
			if (auto pvalue_t = exporter.implicit_explicit[idx])
			{
				auto & value_t = *pvalue_t;

				WriteKey(keymap, archive, indent, value_t, idx == 0, itr.key.id);

				archive.Push(' ');

				Write(archive, value_t.to_string[exporter.isarray](keymap, itr.value));

				auto linebreak = Extend(archive, 3);

				linebreak[0] = ';';
				linebreak[1] = 10;
				linebreak[2] = 10;

				written = true;

				break;
			}
		}
	}

	for (auto & i : m_map_types)
	{
		auto & map_t = i.value;

		auto implicit = map_t.type_ids[0] == REFLEX_TYPEID(PropertySet);

		for (auto & p : propertyset.Iterate(map_t.type_ids[0]))
		{
			WriteKey(keymap, archive, indent, map_t, implicit, p.key.id);

			archive.Push(10);

			WriteNode(Cast<PropertySet>(p.value), state, archive, ';');
		}

		for (auto & p : propertyset.Iterate(map_t.type_ids[1]))
		{
			auto & values = Cast<PropertySetArray>(p.value)->value;

			WriteKey(keymap, archive, indent, map_t, implicit, p.key.id);

			if (values)
			{
				archive.Push(10);

				WriteLine(archive, Join(state.indent, '['));

				state.indent.Push(9);

				for (auto & ii : values) WriteNode(ii, state, archive, ',');

				state.indent.Pop();

				WriteLine(archive, Join(state.indent, "];"));
				
				archive.Push(10);
			}
			else
			{
				WriteLine(archive, Join(" [];"));

				archive.Push(10);
			}
		}
	}



	////TOOD make this recursive to resolve arrays

	//for (auto & itr : propertyset.Iterate(REFLEX_TYPEID(GenericObjectArray)))
	//{
	//	auto & objects = Cast<GenericObjectArray>(*itr.value);

	//	if (objects)
	//	{
	//		auto type_id = objects.GetFirst()->classinfo->type_id;

	//		for (auto & i : objects)
	//		{
	//			if (SetFiltered(type_id, i->classinfo->type_id))
	//			{
	//				//mutli variate

	//				goto Next;
	//			}
	//		}

	//		//uni variate

	//		for (auto & i : m_value_types)
	//		{
	//			auto & value_t = i.value;

	//			if (type_id == *value_t.array_rttid)
	//			{
	//				CString line;

	//				for (auto & ii : objects)
	//				{
	//					line = Join(line, "[", value_t.array2string(keymap, ii), "], ");
	//				}

	//				if (line) line = ReverseSplice(line, 2).a;

	//				WriteLine(archive, Join(indent, MakeKey(keymap, itr.key.id), ": ", "[", line, "];"));

	//				WriteLine(archive);

	//				written = true;
	//			}
	//		}
	//	}

	//	REFLEX_MARKER(Next);
	//}

	return written;
}

void Reflex::Data::Write(Data::Archive & archive, const CString::View & string)
{
	archive.Append(Pack(string));
}


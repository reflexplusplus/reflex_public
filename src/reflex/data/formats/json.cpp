#include "json.h"
#include "json/tokeniser.cpp"




//
//JSON

REFLEX_BEGIN_INTERNAL(Reflex::Data)

constexpr CString::View kColon = ": ";

REFLEX_NOINLINE void JsonFormat::OutputKey(const CString::View & key, Archive & out)
{
	out.Push(kDoubleQuote);

	out.Append(Pack(key));

	auto close = Extend(out, 3);

	close[0] = kDoubleQuote;
	close[1] = ':';
	close[2] = ' ';
}

void JsonFormat::OutputLevel(const KeyMap & keymap, const PropertySet & in, Archive & out) const
{
	constexpr auto FindValueHandler = [](const Array <ValueTypeHandler> & handlers, TypeID type_id) -> Tuple <const ValueTypeHandler*,UInt>
	{
		for (auto & i : handlers)
		{
			if (auto idx = Search(i.type_ids, type_id))
			{
				return {&i, idx.value };
			}
		}

		return {};
	};

	out.Push('{');

	auto size = out.GetSize();

	TypeID type_id = 0;

	Pair <const ValueTypeHandler*,UInt> handler;

	for (auto & i : in.Iterate())
	{
		if (SetFiltered(type_id, i.key.type_id))
		{
			handler = FindValueHandler(m_valuehandlers, type_id);
		}

		if (handler.a)
		{
			if (auto key = GetKey(keymap, i.key.id))
			{
				OutputKey(key, out);

				Write(out, handler.a->to_string[handler.b](keymap, i.value));

				Write(out, ", ");
			}
			else
			{
				REFLEX_ASSERT_EX(false, "Data::kJsonFormat missing key");
			}
		}
	}

	if (SetFiltered(size, out.GetSize()))
	{
		out.Shrink(2);
	}

	out.Push('}');
}

JsonFormat::JsonFormat()
{
	auto & propertyset_t = m_valuehandlers.Push();

	propertyset_t.type_ids[0] = REFLEX_TYPEID(PropertySet);
	propertyset_t.type_ids[1] = REFLEX_TYPEID(PropertySetArray);

	propertyset_t.to_string[0] = [](const KeyMap & keymap, const Object & object) -> CString 
	{
		Data::Archive out;

		Cast<JsonFormat>(kJsonFormat)->OutputLevel(keymap, Cast<PropertySet>(object), out);

		return Data::Unpack<CString::View>(out);
	};

	propertyset_t.to_string[1] = [](const KeyMap & keymap, const Object & object)
	{
		Data::Archive out;

		if (auto & values = Cast<PropertySetArray>(object)->value)
		{
			for (auto & i : values)
			{
				Cast<JsonFormat>(kJsonFormat)->OutputLevel(keymap, i, out);

				Write(out, kArrayComma);
			}

			out.Shrink(2);
		}

		return Join('[', Data::Unpack<CString::View>(out), ']');
	};

	m_valuehandlers.Push(kPropertySheetBool);
	m_valuehandlers.Push(kPropertySheetUInt32);
	m_valuehandlers.Push(kPropertySheetUInt64);
	m_valuehandlers.Push(kPropertySheetInt32);
	m_valuehandlers.Push(kPropertySheetInt64);
	m_valuehandlers.Push(kPropertySheetFloat32);
	m_valuehandlers.Push(kPropertySheetFloat64);
	m_valuehandlers.Push(kPropertySheetCString);
	m_valuehandlers.Push(kPropertySheetWString);

	auto & object_t = m_valuehandlers.Push();

	object_t.to_string[0] = [](const KeyMap & keymap, const Object & object) -> CString
	{
		return kNull;
	};

	object_t.to_string[1] = [](const KeyMap & keymap, const Object & object)
	{
		Data::Archive out;

		if (auto & values = Cast<ObjectArray<Object>>(object)->value)
		{
			for ([[maybe_unused]] auto & i : values)
			{
				Write(out, kNull);
				
				Write(out, kArrayComma);
			}

			out.Shrink(2);
		}

		return Join('[', Data::Unpack<CString::View>(out), ']');
	};
}

bool JsonFormat::SupportsType(TypeID type_id) const
{
	for (auto & i : m_valuehandlers)
	{
		if (Search(i.type_ids, type_id)) return true;
	}

	return type_id == REFLEX_TYPEID(KeyMap);
}

void JsonFormat::OnReset(PropertySet & node) const
{
	for (auto & i : m_valuehandlers)
	{
		node.UnsetAll(i.type_ids[0]);

		node.UnsetAll(i.type_ids[1]);
	}
}

bool JsonFormat::OnEncode(Archive & out, const PropertySet & in, UInt options) const
{
	typedef ObjectArray <Object> Objects;

	auto keymap = GetKeyMap(in);

	if (auto nodes = GetProperty<PropertySetArray>(in, kNullKey))
	{
		auto output = m_valuehandlers.GetFirst().to_string[true](keymap, nodes);

		out.Append(Data::Pack(output));
	}
	else
	{
		OutputLevel(keymap, in, out);
	}

	return true;
}

bool JsonFormat::OnDecode(PropertySet & out, const Archive::View & in, UInt flags) const
{
	if (auto text = Trim(Reinterpret<CString::View>(in)))
	{
		JsonTokeniser::State state(AcquireKeyMap(out), UInt8(flags));

		UInt16 outer = Reinterpret<UInt16>(MakeTuple(text.GetFirst(), text.GetLast()));

		text = Mid<true>(text, 1, text.size - 2);

		try
		{
			UInt line;

			if (outer == 32123)
			{
				JsonTokeniser::ObjectImpl root(state, out);

				Detail::Tokeniser::Tokenise(root, text, line);

				if (root.m_value) root.CommitValue();

				return true;
			}
			else if (outer == 23899)
			{
				JsonTokeniser::ArrayImpl root(state);

				Detail::Tokeniser::Tokenise(root, text, line);

				auto object_with_type = root.ConvertArray(0);

				REFLEX_ASSERT(object_with_type.b == REFLEX_TYPEID(PropertySetArray));

				out.SetProperty({ kNullKey, object_with_type.b }, object_with_type.a);

				return true;
			}
		}
		catch (ParseError & error)
		{
			SetError(out, error.a, {}, std::move(error.b));
		}
	}

	return false;
}

REFLEX_END_INTERNAL

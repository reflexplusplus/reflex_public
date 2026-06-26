#include "tokeniser.h"
#include "../json.h"




//
//JSON

#define JSON_THROW(line, msg) DoThrow(MakeTuple(UInt(line),CString(msg)))
#define JSON_ASSERT(line, test, msg) if (!(test)) { JSON_THROW(line, msg); }
#define JSON_EXPECT(line, expect, t) JSON_ASSERT(line, expect == t, REFLEX_STRINGIFY(t))

REFLEX_BEGIN_INTERNAL(Reflex::Data)

void DoThrow(Tuple <Reflex::UInt,Reflex::CString> p)
{
	throw(p);
}

constexpr CString::View kErrorUnexpectedScope = "unexpected scope";
constexpr CString::View kErrorUnexpectedSymbol = "unexpected symbol";

const ValueTypeHandler kJsonFormatWordHandler =
{
	"",
	{},
	[](KeyMap&, const CString::View & word)
	{
		switch (Key32(word).value)
		{
		case K32("true"):
		case K32("TRUE"):
			return Detail::CreateObjectWithType<BoolProperty>(true);

		case K32("false"):
		case K32("FALSE"):
			return Detail::CreateObjectWithType<BoolProperty>(false);

		default:
			//case K32("null"):
			//case K32("NULL"):
			return Detail::MakeObjectWithType(Object::null);
		}
	},
	[](KeyMap & keymap, const Array <CString::View> & words)
	{
		constexpr auto IsBool = [](Key32 word)
		{
			switch (word.value)
			{
			case K32("true"):
			case K32("TRUE"):
			case K32("false"):
			case K32("FALSE"):
				return true;

			default:
				return false;
			}
		};

		auto is_bool = IsBool(words.GetFirst());

		for (auto & i : Mid<true>(words, 1))
		{
			if (SetFiltered(is_bool, IsBool(i)))
			{
				JSON_THROW(0, kErrorUnexpectedSymbol);
			}
		}

		if (is_bool)
		{
			auto object_with_type = Detail::CreateObjectWithType<ArrayOfBoolProperty>(words.GetSize());

			auto & bools = Cast<ArrayOfBoolProperty>(object_with_type.a)->value;

			REFLEX_LOOP(idx, words.GetSize())
			{
				bools[idx] = Detail::Stringizer<bool>::FromString(keymap, words[idx]);
			}

			return object_with_type;
		}
		else
		{
			return Detail::CreateObjectWithType<ObjectArray<Object>>(words.GetSize());
		}
	}
};

JsonTokeniser::State::State(KeyMap & keymap, UInt8 flags)
	: keymap(keymap)
{
	REFLEX_LOOP(idx, kNumValueType)
	{
		token2handler[idx] = 0;
	}

	token2handler[kValueTypeInt] = (flags & kJsonFormatOptionInt64) ? &kPropertySheetInt64 : &kPropertySheetInt32;
	
	token2handler[kValueTypeFloat] = (flags & kJsonFormatOptionFloat64) ? &kPropertySheetFloat64 : &kPropertySheetFloat32;
	
	token2handler[kValueTypeDoubleQuotedString] = (flags & kJsonFormatOptionWString) ? &kPropertySheetWString : &kPropertySheetCString;
	
	token2handler[kValueTypeWord] = &kJsonFormatWordHandler;
}

JsonTokeniser::JsonTokeniser(State & state, Expect init)
	: state(state),
	m_expect(init)
{
}

REFLEX_INLINE Reference <Object> JsonTokeniser::CreateValue(UInt line, ValueType type, const CString::View & word)
{
	JSON_EXPECT(line, m_expect, kExpectValue);

	m_expect = kExpectComma;

	return state.token2handler[type]->object_ctr(state.keymap, ConsumeSign(word)).a;
}

REFLEX_INLINE CString::View JsonTokeniser::ConsumeSign(const CString::View & word)
{
	auto rtn = word;

	if (m_psign)
	{
		rtn = { m_psign, word.size + UInt(word.data - m_psign) };

		m_psign = 0;
	}

	return rtn;
}

JsonTokeniser::ObjectImpl::ObjectImpl(State & state, TRef <PropertySet> dynamic)
	: JsonTokeniser(state, kExpectKey),
	dynamic(dynamic)
{
}

void JsonTokeniser::ObjectImpl::CommitValue()
{
	Address adr = { m_key, m_value->object_t->type_id };

	dynamic->SetProperty(adr, m_value);

	m_value.Clear();

	m_expect = kExpectKey;
}

TRef <Detail::Tokeniser> JsonTokeniser::ObjectImpl::OnPush(UInt line, const char & bracket)
{
	JSON_EXPECT(line, m_expect, kExpectValue);

	if (bracket == '{')
	{
		auto child = REFLEX_CREATE(PropertySet);

		m_value = child;

		return REFLEX_CREATE(ObjectImpl, state, child);
	}
	else if (bracket == '[')
	{
		return REFLEX_CREATE(ArrayImpl, state);
	}

	JSON_THROW(line, kErrorUnexpectedScope);

	return *this;
}

void JsonTokeniser::ObjectImpl::OnPop(UInt line, const char & bracket, Detail::Tokeniser & child)
{
	JSON_ASSERT(line, m_expect == kExpectComma || m_expect == kExpectValue, kErrorUnexpectedSymbol);

	if (bracket == '}')
	{
		auto nested = Cast<ObjectImpl>(child);

		if (nested->m_value) nested->CommitValue();
	}
	else if (bracket == ']')
	{
		m_value = Cast<ArrayImpl>(child)->ConvertArray(line).a;
	}
	else
	{
		JSON_THROW(line, kErrorUnexpectedSymbol);
	}

	m_expect = kExpectComma;
}

void JsonTokeniser::ObjectImpl::OnValue(UInt line, ValueType type, const CString::View & word)
{
	if (m_expect == kExpectKey)
	{
		JSON_ASSERT(line, type == kValueTypeDoubleQuotedString, "expected string");

		m_key = word;

		state.keymap->value.Set(m_key, word);

		m_expect = kExpectColon;
	}
	else
	{
		m_value = CreateValue(line, type, word);
	}
}

void JsonTokeniser::ObjectImpl::OnSymbol(UInt line, const char & symbol)
{
	if (m_expect == kExpectColon)
	{
		m_expect = kExpectValue;
	}
	else if (m_expect == kExpectComma)
	{
		CommitValue();
	}
	else if (m_expect == kExpectValue && symbol == '-')
	{
		m_psign = &symbol;
	}
	else
	{
		JSON_THROW(line, kErrorUnexpectedSymbol);
	}
}

JsonTokeniser::ArrayImpl::ArrayImpl(State & state)
	: JsonTokeniser(state, kExpectValue)
{
}

TRef <Detail::Tokeniser> JsonTokeniser::ArrayImpl::OnPush(UInt line, const char & bracket)
{
	if (bracket == '{')
	{
		auto child = REFLEX_CREATE(PropertySet);

		m_pending_propertysets.Push(child);

		return REFLEX_CREATE(ObjectImpl, state, child);
	}

	JSON_THROW(line, kErrorUnexpectedScope);

	return this;
}

void JsonTokeniser::ArrayImpl::OnPop(UInt line, const char & bracket, Tokeniser & child)
{
	if (bracket == '}')
	{
		auto nested = Cast<ObjectImpl>(child);

		if (nested->m_value) nested->CommitValue();

		m_expect = kExpectComma;
	}
}

void JsonTokeniser::ArrayImpl::OnValue(UInt line, ValueType type, const CString::View & word)
{
	JSON_EXPECT(line, m_expect, kExpectValue);

	if (m_pending_values_type != type)
	{
		switch (m_pending_values_type)
		{
		case kValueTypeInt:
			JSON_ASSERT(line, type == kValueTypeFloat, kErrorUnexpectedSymbol);
			m_pending_values_type = kValueTypeFloat;
			break;

		case kValueTypeFloat:
			JSON_ASSERT(line, type == kValueTypeInt, kErrorUnexpectedSymbol);
			m_pending_values_type = kValueTypeInt;
			break;

		case kValueTypeDoubleQuotedString:
			JSON_ASSERT(line, type == kValueTypeDoubleQuotedString, kErrorUnexpectedSymbol);
			break;

		case kNumValueType:
			m_pending_values_type = type;
			break;

		default:
			JSON_THROW(line, kErrorUnexpectedSymbol);
			break;
		}
	}

	m_pending_values.Push(ConsumeSign(word));

	m_expect = kExpectComma;
}

void JsonTokeniser::ArrayImpl::OnSymbol(UInt line, const char & symbol)
{
	if (m_expect == kExpectValue && symbol == '-')
	{
		m_psign = &symbol;
	}
	else
	{
		JSON_ASSERT(line, symbol == ',' && m_expect == kExpectComma, kErrorUnexpectedSymbol);

		m_expect = kExpectValue;
	}
}

Detail::PropertySheetInterface::ObjectWithType JsonTokeniser::ArrayImpl::ConvertArray(UInt line)
{
	if (m_pending_values)
	{
		return state.token2handler[m_pending_values_type]->array_ctr(state.keymap, m_pending_values);
	}
	else 
	{
		auto typed = New<PropertySetArray>();

		if (m_pending_propertysets)
		{
			typed->value = std::move(m_pending_propertysets);
		}

		return Detail::MakeObjectWithType(*typed);
	}
}

REFLEX_END_INTERNAL

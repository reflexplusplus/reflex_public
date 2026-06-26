#include "formatimpl.h"
#include "../../../../../include/reflex/data/format/formats/propertysheet.h"
#include "../../tokeniser.h"




//
//

#define PROPERTYSHEET_THROW(line, msg) throw(MakeTuple(UInt(line),CString(msg)))
#define PROPERTYSHEET_ASSERT(test, line, msg) if (!(test)) { PROPERTYSHEET_THROW(line, msg); }

#if REFLEX_DEBUG
#define PROPERTYSHEET_THROW_UNEXPECTED_SYMBOL(line, symbol) throw(MakeTuple(UInt(line),Join(kErrorUnexpectedSymbol, ' ', symbol)))
#else
#define PROPERTYSHEET_THROW_UNEXPECTED_SYMBOL(line, symbol) throw(MakeTuple(UInt(line),CString(kErrorUnexpectedSymbol)))
#endif

REFLEX_BEGIN_INTERNAL(Reflex::Data::Detail)

DATA_DECLARE_ERROR(ExpectedID, "expected id");
DATA_DECLARE_ERROR(ExpectedType, "expected type");
DATA_DECLARE_ERROR(ExpectedValue, "expected value");
DATA_DECLARE_ERROR(ExpectedDelimiter, "expected , or ;");
DATA_DECLARE_ERROR(UnsupportedType, "unsupported type");
DATA_DECLARE_ERROR(UnsupportedOption, "unsupported option");

constexpr UInt32 kFalseTrue[] = { K32("false"), K32("true") };

struct CommonParser;

struct AbstractParser : public Tokeniser
{
	AbstractParser(bool preprocessor)
		: preprocessor(preprocessor)
	{
	}

	const bool preprocessor = false;
};

struct PreprocessorParser : public AbstractParser
{
	PreprocessorParser(CommonParser & parent, bool condition);


	void OnComment(UInt line, const CString::View & text) override;

	TRef <Tokeniser> OnPush(UInt line, const char & bracket) override;

	void OnPop(UInt line, const char & bracket, Tokeniser & child) override;

	void OnValue(UInt line, ValueType type, const CString::View & value) override;

	void OnSymbol(UInt line, const char & symbol) override;


	CommonParser & parent;

	const bool m_condition;
};

struct ConditionParser : public AbstractParser
{
	ConditionParser();


	void OnComment(UInt line, const CString::View & text) override {}

	TRef <Tokeniser> OnPush(UInt line, const char & bracket) override;

	void OnPop(UInt line, const char & bracket, Tokeniser & child) override;

	void OnValue(UInt line, ValueType type, const CString::View & value) override;

	void OnSymbol(UInt line, const char & symbol) override;


	Array < Pair <PropertySheetInterface::TokenType,CString::View> > m_tokens;
};

struct CommonParser : public AbstractParser
{
	enum Stage
	{
		kExpectTypeOrIdOrPreprocessor,
		kExpectType,
		kExpectID,
		kExpectAssignmentOperator,
		kExpectReference,
		kExpectNumberValue,
		kExpectValue,
		kExpectDelimiter,

		kExpectPreprocessor,

		kExpectConditionIf,
		kExpectConditionBody,

		kExpectOptionKey,
		kExpectOptionValue
	};

	typedef Array < TRef <PropertySet> > Stack;

	CommonParser(Allocator & allocator, PropertySheetInterface & iface, KeyMap & keymap, Stack & stack);


	virtual void DoPop(UInt line) = 0;

	void OnComment(UInt line, const CString::View & text) override;

	TRef <Tokeniser> OnPush(UInt line, const char & symbol) override;

	void OnPop(UInt line, const char & bracket, Tokeniser & child) override;

	void Extend(UInt line, const CString::View & b)
	{
		auto & a = m_pending_values.GetLast();

		auto start = a.data;

		auto end = b.data + b.size;

		a.size = UInt(end - start);
	}


	Allocator & allocator;

	PropertySheetInterface & iface;

	KeyMap & keymap;

	Stack & stack;

	Stage m_next;

	UInt8 m_if_passed;

	bool m_if_result;

	bool m_if_or;

	PropertySheetInterface::TokenType m_pending_values_type;

	Array <CString::View> m_pending_values;

	Array <PropertySheetInterface::ObjectWithType> m_pending_objects;

	Key32 m_key;

	CString::View m_pending_object_t;
};

struct ObjectParser : public CommonParser
{
	ObjectParser(Allocator & allocator, PropertySheetInterface & iface, KeyMap & keymap, Stack & stack);

	ObjectParser(Allocator & allocator, PropertySheetInterface & iface, KeyMap & keymap, Stack & stack, PropertySheetInterface::ObjectWithType && object);

	~ObjectParser();


	void DoPop(UInt line) override;

	void OnSymbol(UInt line, const char & symbol) override;

	void OnValue(UInt line, ValueType type, const CString::View & value) override;

	void CommitPending(UInt line);


	Reference <PropertySet> m_dynamic;

	TypeID m_dynamic_rttid;
};

struct ConstructorParser : public ObjectParser
{
	using ObjectParser::ObjectParser;

	void DoPop(UInt line) override
	{
		bool test = !m_pending_objects && !m_pending_values && (m_next == kExpectTypeOrIdOrPreprocessor || m_next == kExpectDelimiter);

		//REFLEX_ASSERT(test);

		PROPERTYSHEET_ASSERT(test, line, kErrorExpectedDelimiter);
	}
};

struct ArrayParser : public CommonParser
{
	ArrayParser(Allocator & allocator, PropertySheetInterface & iface, KeyMap & keymap, Stack & stack);


	void DoPop(UInt line) override;

	void OnValue(UInt line, ValueType type, const CString::View & value) override;

	void OnSymbol(UInt line, const char & symbol) override;
};

CommonParser::CommonParser(Allocator & allocator, PropertySheetInterface & iface, KeyMap & keymap, Stack & stack)
	: AbstractParser(false),
	allocator(allocator),
	iface(iface),
	keymap(keymap),
	stack(stack),
	m_next(kExpectTypeOrIdOrPreprocessor)
{
}

void CommonParser::OnComment(UInt line, const CString::View & text)
{
	//auto comment = New<PropertySheetInterface::CommentObject>();

	//comment->line = line;
	//comment->text = text;

	//stack.GetLast()->SetProperty(MakeAddress<PropertySheetInterface::CommentObject>(line), comment);
}

TRef <Detail::Tokeniser> CommonParser::OnPush(UInt line, const char & symbol)
{
	try
	{
		switch (symbol)
		{
		case '{':
			if (m_next == kExpectValue)
			{
				m_next = kExpectDelimiter;
				auto object = iface.CreateObject(stack.GetLast(), m_pending_object_t, m_key, false);
				return REFLEX_CREATE_EX(allocator, ObjectParser, allocator, iface.GetInterface(object), keymap, stack, std::move(object));
			}
			else if (m_next == kExpectConditionBody)
			{
				auto parser = REFLEX_CREATE_EX(allocator, PreprocessorParser, *this, !m_if_passed && m_if_result);
				m_next = kExpectTypeOrIdOrPreprocessor;
				return parser;
			}
			else
			{
				PROPERTYSHEET_THROW(line, kErrorExpectedValue);
			}

		case '[':
			PROPERTYSHEET_ASSERT(m_next == kExpectValue, line, kErrorExpectedValue);
			m_next = kExpectDelimiter;
			return REFLEX_CREATE_EX(allocator, ArrayParser, allocator, iface, keymap, stack);

		case '(':
			if (m_next == kExpectConditionIf)
			{
				m_next = kExpectConditionBody;
				return REFLEX_CREATE_EX(allocator, ConditionParser);
			}
			else
			{
				PROPERTYSHEET_ASSERT(m_next == kExpectDelimiter, line, kErrorExpectedDelimiter);
				PROPERTYSHEET_ASSERT(m_pending_values_type == PropertySheetInterface::kTokenTypeWord && m_pending_values.GetSize() == 1, line, "Expected function name");
				auto object = iface.CreateObject(stack.GetLast(), m_pending_values.GetFirst(), m_key, false);
				auto parser = REFLEX_CREATE_EX(allocator, ConstructorParser, allocator, iface.GetInterface(object), keymap, stack, std::move(object));
				m_pending_values.Clear();
				return parser;
			}
		};
	}
	catch (const CString & error)
	{
		throw MakeTuple(line, error);
	}

	return {};
}

void CommonParser::OnPop(UInt line, const char & symbol, Tokeniser & child_)
{
	if (symbol == '}')
	{
		if (Cast<AbstractParser>(child_)->preprocessor)
		{
			m_if_passed += m_if_result;

			m_next = kExpectTypeOrIdOrPreprocessor;

			return;
		}
		else
		{
			auto child = Cast<ObjectParser>(child_);

			m_pending_objects.Push({ child->m_dynamic, child->m_dynamic_rttid });

			m_next = kExpectDelimiter;
		}
	}
	else if (symbol == ']')
	{
		PROPERTYSHEET_ASSERT(!m_pending_values, line, kErrorParseError);

		auto child = Cast<ArrayParser>(child_);

		if (child->m_pending_values)
		{
			m_pending_objects.Push(iface.CreateValueArray(keymap, m_pending_object_t, child->m_pending_values_type, child->m_pending_values));
		}
		else
		{
			m_pending_objects.Push(iface.CreateObjectArray(m_pending_object_t, child->m_pending_objects));

			if (REFLEX_DEBUG) child->m_pending_objects.Clear();
		}

		m_next = kExpectDelimiter;
	}
	else if (symbol == ')')
	{
		if (Cast<AbstractParser>(child_)->preprocessor)
		{
			if (m_if_or && m_if_result == false)
			{
				m_if_result = iface.OnCondition(Cast<ConditionParser>(child_)->m_tokens);
			}
			else if (!m_if_or && m_if_result)
			{
				m_if_result = iface.OnCondition(Cast<ConditionParser>(child_)->m_tokens);
			}
		
			m_next = kExpectConditionBody;

			return;
		}
		else
		{
			auto child = Cast<ConstructorParser>(child_);

			if (m_next == kExpectDelimiter) child->CommitPending(line);

			m_pending_objects.Push({ child->m_dynamic, child->m_dynamic_rttid });

			m_next = kExpectDelimiter;
		}
	}
	else
	{
		REFLEX_ASSERT(false);
		return;
	}

	Cast<CommonParser>(child_)->DoPop(line);
}

ObjectParser::ObjectParser(Allocator & allocator, PropertySheetInterface & iface, KeyMap & keymap, Stack & stack)
	: CommonParser(allocator, iface, keymap, stack),
	m_dynamic(stack.GetFirst()),
	m_dynamic_rttid(m_dynamic->object_t->type_id)
{
}

ObjectParser::ObjectParser(Allocator & allocator, PropertySheetInterface & iface, KeyMap & keymap, Stack & stack, PropertySheetInterface::ObjectWithType && object)
	: CommonParser(allocator, iface, keymap, stack),
	m_dynamic(Cast<PropertySet>(object.a)),
	m_dynamic_rttid(object.b)
{
	stack.Push(m_dynamic);
}

ObjectParser::~ObjectParser()
{
	REFLEX_ASSERT(stack.GetLast() == m_dynamic);

	stack.Pop();
}

void ObjectParser::DoPop(UInt line)
{
	//REFLEX_ASSERT(!m_pending_objects && !m_pending_values && m_next == kExpectTypeOrID);

	PROPERTYSHEET_ASSERT(!m_pending_objects && !m_pending_values && m_next == kExpectTypeOrIdOrPreprocessor, line, kErrorExpectedDelimiter);
}

void ObjectParser::OnValue(UInt line, ValueType type, const CString::View & value)
{
	auto ParseID = [this](UInt line, ValueType type, const CString::View & value)
	{
		RegisterKey(keymap, value);

		switch (type)
		{
		case kValueTypeWord:
		case kValueTypeSingleQuotedString:
			RegisterKey(keymap, value);
			m_key = MakeKey32(value);
			break;

		case kValueTypeInt:
			m_key = ToUInt32(value);
			break;

		case kValueTypeHex:
			REFLEX_IF_DEBUG(File::output.Warn("legacy $hex key will be dropped"));
			m_key = Data::Unpack<Key32>(Data::HexToBytes(value));
			break;

		default:
			PROPERTYSHEET_THROW(line, kErrorExpectedID);
		}

		m_next = kExpectAssignmentOperator;
	};

	switch (m_next)
	{
	case kExpectTypeOrIdOrPreprocessor:
		return ParseID(line, type, value);

	case kExpectType:
		m_pending_object_t = value;
		m_next = kExpectID;
		break;

	case kExpectID:
		return ParseID(line, type, value);

	case kExpectValue:
		switch (type)
		{
		case kValueTypeWord:
		case kValueTypeSingleQuotedString:
			RegisterKey(keymap, value);
			if (Search(ToView(Reflex::Detail::kFalseTrue), value))
			{
				m_pending_values_type = PropertySheetInterface::kTokenTypeBool;
			}
			else
			{
				m_pending_values_type = PropertySheetInterface::kTokenTypeWord;
			}
			break;

		default:
			m_pending_values_type = PropertySheetInterface::TokenType(type);
			break;
		};
		m_pending_values.Push(value);
		m_next = kExpectDelimiter;
		break;

	case kExpectNumberValue:
		PROPERTYSHEET_ASSERT(Inside<Int>(type, kValueTypeInt, 2), line, kErrorExpectedValue);
		m_pending_values_type = PropertySheetInterface::TokenType(type);
		Extend(line, value);
		m_next = kExpectDelimiter;
		break;

	case kExpectDelimiter:
		PROPERTYSHEET_ASSERT(m_pending_values && type == kValueTypeWord, line, kErrorExpectedValue);
		Extend(line, value);
		break;

	case kExpectReference:
		PROPERTYSHEET_ASSERT(type == kValueTypeWord, line, kErrorExpectedID);
		RegisterKey(keymap, value);
		m_pending_values_type = PropertySheetInterface::kTokenTypeReference;
		m_pending_values.Push(value);
		m_next = kExpectDelimiter;
		break;

	case kExpectPreprocessor:
		if (type == kValueTypeWord)
		{
			switch (MakeKey32(value))
			{
			case K32("if"):
				m_if_passed = 0;
				m_if_result = false;
				m_if_or = true;
				m_next = kExpectConditionIf;
				break;

			case K32("elif"):
				m_if_result = false;
				m_if_or = true;
				m_next = kExpectConditionIf;
				break;

			case K32("else"):
				m_if_result = !m_if_passed;
				m_if_or = true;
				m_next = kExpectConditionBody;
				break;

			case K32("option"):
				m_next = kExpectOptionKey;
				break;

			default:
				PROPERTYSHEET_THROW(line, kErrorParseError);
				break;
			}
		}
		break;

	case kExpectConditionBody:
		PROPERTYSHEET_ASSERT(type == kValueTypeWord, line, kErrorExpectedValue);
		switch (MakeKey32(value))
		{
		case K32("or"):
			m_if_or = true;
			m_next = kExpectConditionIf;
			break;

		case K32("and"):
			m_if_or = false;
			m_next = kExpectConditionIf;
			break;
		}
		break;

	case kExpectOptionKey:
		m_next = kExpectOptionValue;
		m_key = value;
		break;

	case kExpectOptionValue:
		PROPERTYSHEET_ASSERT(iface.OnSetOption(m_key, value), line, kErrorUnsupportedOption);
		m_next = kExpectTypeOrIdOrPreprocessor;
		break;

	default:
		PROPERTYSHEET_THROW(line, kErrorParseError);
		break;
	}
}

void ObjectParser::OnSymbol(UInt line, const char & symbol)
{
	switch (m_next)
	{
	case kExpectAssignmentOperator:
		switch (symbol)
		{
		case ':':
			m_next = kExpectValue;
			break;

		case ';':
		{
			auto parent = stack.GetLast();
			auto value = iface.CreateObject(parent, m_pending_object_t, m_key, true);
			parent->SetProperty({ m_key, value.b }, value.a);
			m_pending_object_t = {};
			m_key = kNullKey;
			m_next = kExpectTypeOrIdOrPreprocessor;
		}
			break;

		default:
			PROPERTYSHEET_THROW(line, "expected :");
			break;
		}
		break;

	case kExpectDelimiter:
		if (symbol == ',' || symbol == '>')
		{
			m_next = kExpectValue;
		}
		else if (symbol == ';')
		{
			PROPERTYSHEET_ASSERT(Or(m_pending_values, m_pending_objects), line, "expected value(s)");
		
			CommitPending(line);

			m_pending_object_t = {};

			m_key = kNullKey;

			m_next = kExpectTypeOrIdOrPreprocessor;
		}
		else if (symbol == '-' || symbol == '.')
		{
			if (m_next == kExpectAssignmentOperator || m_next == kExpectDelimiter)
			{
				if (m_pending_values) Extend(line, { &symbol, 1 });
			}
			else
			{
				PROPERTYSHEET_ASSERT(false, line, kErrorExpectedDelimiter);
			}
		}
		else if (symbol == '|')
		{
			PROPERTYSHEET_ASSERT(Or(m_pending_values, m_pending_objects), line, "expected value(s)");

			CommitPending(line);

			m_next = kExpectValue;
		}
		else
		{
			PROPERTYSHEET_THROW_UNEXPECTED_SYMBOL(line, symbol);
		}
		break;

	case kExpectTypeOrIdOrPreprocessor:
		if (symbol == '@')
		{
			m_next = kExpectType;
		}
		else if (symbol == '#')
		{
			m_next = kExpectPreprocessor;
		}
		else
		{
			PROPERTYSHEET_THROW_UNEXPECTED_SYMBOL(line, symbol);
		}
		break;

	case kExpectID:
		PROPERTYSHEET_ASSERT(And(symbol == ':', m_pending_object_t), line, kErrorUnexpectedSymbol);
		m_next = kExpectValue;
		break;

	case kExpectValue:
		switch (symbol)
		{
		case '-':
			m_pending_values.Push({ &symbol, 1 });
			m_next = kExpectNumberValue;
			break;

		case '&':
			m_next = kExpectReference;
			break;

		default:
			PROPERTYSHEET_THROW_UNEXPECTED_SYMBOL(line, symbol);
			break;
		}
		break;

	default:
		PROPERTYSHEET_THROW_UNEXPECTED_SYMBOL(line, symbol);
		break;
	}
}

void ObjectParser::CommitPending(UInt line)
{
	constexpr auto Store = [](ObjectParser & self, PropertySheetInterface::ObjectWithType && value)
	{
		self.stack.GetLast()->SetProperty({ self.m_key, value.b }, value.a);
	};

	try
	{
		if (auto & values = m_pending_values)
		{
			if (values.GetSize() > 1)
			{
				REFLEX_IF_DEBUG(PROPERTYSHEET_ASSERT(m_pending_values_type != PropertySheetInterface::kTokenTypeReference, line, kErrorExpectedValue));

				Store(*this, iface.CreateValueArray(keymap, m_pending_object_t, m_pending_values_type, values));
			}
			else
			{
				Store(*this, iface.CreateValue(keymap, m_pending_object_t, m_pending_values_type, values.GetFirst()));
			}

			values.Clear();
		}
		else if (auto & objects = m_pending_objects)
		{
			if (objects.GetSize() > 1)
			{
				Store(*this, iface.CreateObjectArray(m_pending_object_t, std::move(objects)));
			}
			else
			{
				Store(*this, std::move(objects.GetFirst()));
			}

			objects.Clear();
		}
	}
	catch (const CString & error)
	{
		throw MakeTuple(line, error);
	}
};

PreprocessorParser::PreprocessorParser(CommonParser & parent, bool condition)
	: AbstractParser(true),
	parent(parent),
	m_condition(condition)
{
}

void PreprocessorParser::OnComment(UInt line, const CString::View & text) 
{
	parent.OnComment(line, text); 
}

TRef <Tokeniser> PreprocessorParser::OnPush(UInt line, const char & bracket)
{
	if (m_condition)
	{
		return parent.OnPush(line, bracket);
	}
	else
	{
		return this;
	}
}

void PreprocessorParser::OnPop(UInt line, const char & bracket, Tokeniser & child)
{
	if (m_condition) parent.OnPop(line, bracket, child);
}

void PreprocessorParser::OnValue(UInt line, ValueType type, const CString::View & value)
{
	if (m_condition) parent.OnValue(line, type, value);
}

void PreprocessorParser::OnSymbol(UInt line, const char & symbol)
{
	if (m_condition) parent.OnSymbol(line, symbol);
}

ConditionParser::ConditionParser()
	: AbstractParser(true)
{
}

TRef <Tokeniser> ConditionParser::OnPush(UInt line, const char & bracket)
{
	PROPERTYSHEET_THROW(line, kErrorParseError);

	return this;
}

void ConditionParser::OnPop(UInt line, const char & bracket, Tokeniser & child)
{
}

void ConditionParser::OnValue(UInt line, ValueType type, const CString::View & value)
{
	m_tokens.Push({ PropertySheetInterface::TokenType(type), value });
}

void ConditionParser::OnSymbol(UInt line, const char & symbol)
{
	m_tokens.Push({ PropertySheetInterface::TokenType(0), ToView(symbol) });
}

ArrayParser::ArrayParser(Allocator & allocator, PropertySheetInterface & iface, KeyMap & keymap, Stack & stack)
	: CommonParser(allocator, iface, keymap, stack)
{
	m_next = kExpectValue;
}

void ArrayParser::DoPop(UInt line)
{
	REFLEX_ASSERT(m_next >= kExpectValue);

	PROPERTYSHEET_ASSERT(m_next >= kExpectValue, line, kErrorExpectedValue);
}

void ArrayParser::OnValue(UInt line, ValueType type, const CString::View & value)
{
	switch (m_next)
	{
	case kExpectValue:
		m_pending_values_type = PropertySheetInterface::TokenType(type);
		m_pending_values.Push(value);
		m_next = kExpectDelimiter;
		break;

	case kExpectNumberValue:
		PROPERTYSHEET_ASSERT(Inside<Int>(type, kValueTypeInt, 2), line, kErrorExpectedValue);
		m_pending_values_type = PropertySheetInterface::TokenType(type);
		Extend(line, value);
		m_next = kExpectDelimiter;
		break;

	case kExpectDelimiter:
		PROPERTYSHEET_ASSERT(m_pending_values && type == kValueTypeWord, line, kErrorExpectedValue);
		Extend(line, value);
		break;

	case kExpectType:
		PROPERTYSHEET_ASSERT(type == kValueTypeWord, line, kErrorExpectedValue);
		m_pending_object_t = value;
		m_next = kExpectValue;
		break;

	default:
		PROPERTYSHEET_THROW(line, kErrorParseError);
		break;
	}
}

void ArrayParser::OnSymbol(UInt line, const char & symbol)
{
	switch (m_next)
	{
	case kExpectDelimiter:
		if (symbol == ',')
		{
			m_next = kExpectValue;
		}
		else
		{
			PROPERTYSHEET_THROW_UNEXPECTED_SYMBOL(line, symbol);
		}
		break;

	case kExpectValue:
		if (symbol == '-')
		{
			m_pending_values.Push({ &symbol, 1 });
			m_next = kExpectNumberValue;
		}
		else if (symbol == '@')
		{
			m_next = kExpectType;
		}
		else
		{
			PROPERTYSHEET_THROW_UNEXPECTED_SYMBOL(line, symbol);
		}
		break;

	default:
		PROPERTYSHEET_THROW_UNEXPECTED_SYMBOL(line, symbol);
		break;
	}
}

REFLEX_END_INTERNAL

REFLEX_SET_TRAIT(Data::Detail::ObjectParser, IsSingleThreadExclusive);
REFLEX_SET_TRAIT(Data::Detail::ArrayParser, IsSingleThreadExclusive);

Reflex::Data::PropertySheetFormatImpl::PropertySheetFormatImpl(Detail::PropertySheetInterface & iface)
	: iface(iface)
{
	RegisterType<KeyMap>();
	RegisterType<BoolProperty>();	//for PropertyEditor open
}

bool Reflex::Data::PropertySheetFormatImpl::SupportsType(TypeID type_id) const
{
	return True(m_types.Search(type_id));
}

void Reflex::Data::PropertySheetFormatImpl::OnReset(PropertySet & data) const
{
	for (auto & i : m_types) data.UnsetAll(i.key);
}

bool Reflex::Data::PropertySheetFormatImpl::OnDecode(PropertySet & out, const Archive::View & in, UInt32 options) const
{
	Reflex::Detail::SilentReference <PropertySet> retain(out);

	try
	{
		auto allocator = AutoRelease(CreateAllocator(kRecycleAllocID, Object::null));

		Detail::CommonParser::Stack stack(allocator);

		stack.Push(out);

		if (iface.Begin(out))
		{
			auto keymap = AutoRelease(AcquireKeyMap(out));

			Detail::ObjectParser parser(allocator, iface, keymap, stack);

			UInt line;

			Detail::PropertySheetInterface::LineScope scope(&line);

			Detail::Tokeniser::Tokenise(parser, Reinterpret<CString::View>(in), line);

			parser.DoPop(line);

			parser.iface.End(out);

			if (keymap->value.Empty()) out.UnsetProperty<KeyMap>(kkeymap);

			return true;
		}
		else
		{
			SetError(out, 0, {}, "unsupported type");

			return false;
		}
	}
	catch (ParseError & error)
	{
		SetError(out, error.a, {}, std::move(error.b));

		return false;
	}
}

bool Reflex::Data::PropertySheetFormatImpl::OnEncode(Archive & out, const PropertySet & in, UInt options) const
{
	Reference <ExportState> state = PrepareExport(in);

	Export(in, state, out);

	return true;
}

void Reflex::Data::PropertySheetFormatImpl::WriteNode(const PropertySet & propertyset, ExportState & state, Archive & archive, char delim) const
{
	WriteLine(archive, Join(state.indent, "{"));

	state.indent.Push(9);

	bool written = Export(propertyset, state, archive);

	archive.Shrink(written);

	state.indent.Pop();

	WriteLine(archive, Join(state.indent, '}', delim));

	archive.Push(10);
};

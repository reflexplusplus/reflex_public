#include "tokeniser.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

const Token::Type kTypeWord = Token::kTypeWord;

TRef <Data::Detail::Tokeniser> TransitionalCallbacks::OnPush(UInt line, const char & symbol)
{
	CString::View value(&symbol, 1);

	m_stack.Push(REFLEX_CREATE_EX(allocator, Token, *m_stack.GetLast(), line, Token::kTypeBracket, value, value));

	return this;
}

void TransitionalCallbacks::OnPop(UInt line, const char & symbol, Tokeniser & child)
{
	auto & last = *m_stack.GetLast();

	last.value.size += UInt(&symbol - last.value.data);

	m_stack.Pop();
}

void TransitionalCallbacks::OnValue(UInt line, ValueType type, const CString::View & value)
{
	auto & scope = *m_stack.GetLast();

	switch (type)
	{
	case kValueTypeWord:
		{
			Key32 hash = value;
			REFLEX_CREATE_EX(allocator, Token, scope, line, *TheLibrary::Get()->keywords.SearchValue(hash, &Detail::kTypeWord), value, hash);
		}
		break;

	case kValueTypeInt:
		REFLEX_CREATE_EX(allocator, Token, scope, line, Token::kTypeInt, value, kNullKey);
		break;

	case kValueTypeFloat:
		REFLEX_CREATE_EX(allocator, Token, scope, line, Token::kTypeFloat, value, kNullKey);
		break;

	case kValueTypeDoubleQuotedString:
		REFLEX_CREATE_EX(allocator, Token, scope, line, Token::kTypeDoubleQuotedString, value, value);
		break;

	case kValueTypeSingleQuotedString:
		REFLEX_CREATE_EX(allocator, Token, scope, line, Token::kTypeSingleQuotedString, value, value);
		break;

	case kValueTypeHex:
		REFLEX_CREATE_EX(allocator, Token, scope, line, Token::kTypeHex, value, kNullKey);
		break;
	}
}

void TransitionalCallbacks::OnSymbol(UInt line, const char & symbol)
{
	REFLEX_INLINE_LOCAL(bool, ShouldMergeSymbol)(char a, char b)
	{
		switch (b)
		{
		case ':':
			return a == ':';

		case '=':
		{
			switch (a)
			{
			case '<':
			case '>':
			case '-':
			case '+':
			case '*':
			case '/':
			case '!':
			case '=':
				return true;
			default:
				return false;
			}
		}

		case '+':
		{
			switch (a)
			{
			case '+':
				return true;
			default:
				return false;
			}
		}

		case '-':
		{
			switch (a)
			{
			case '-':
				return true;

			default:
				return false;
			}
		}

		case '|':
			return a == '|';

		case '&':
			return a == '&';

		case '<':
			return a == '<';

		case '>':
			switch (a)
			{
			case '-':
			case '>':
				return true;

			default:
				return false;
			}

		case '.':
			return a == '.';

		default:
			return false;
		}
	}
	REFLEX_END

	auto & scope = *m_stack.GetLast();

	CString::View string = { &symbol, 1 };

	if (auto prev = scope.GetLast())
	{
		auto & value = prev->value;

		if (prev->type == Token::kTypeSymbol && value.data == string.data - 1)
		{
			if (ShouldMergeSymbol::Call(*value.data, *string.data))
			{
				value.size++;

				prev->hash = value;

				return;
			}
		}
	}

	REFLEX_CREATE_EX(allocator, Token, scope, line, Token::kTypeSymbol, string, string);
}

REFLEX_END_INTERNAL





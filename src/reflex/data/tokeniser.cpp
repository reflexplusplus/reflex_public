#include "tokeniser.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::Data::Detail)

DATA_DECLARE_ERROR(UnsupportedLineEnding, "unsupported EOL (CR)");

constexpr char kLF = 10;
constexpr char kCR = 13;

struct NullTokeniserImpl : public Tokeniser
{
	static void Init(CharType char2type[128])
	{
		MemClear(char2type, 128);

		char2type[' '] = kCharTypeWhiteSpace;
		char2type[char(9)] = kCharTypeWhiteSpace;
		char2type[kLF] = kCharTypeLF;
		char2type[kCR] = kCharTypeCR;
		//char2type[char(13)] = kCharTypeWhiteSpace;

		char2type['#'] = kCharTypeSymbol;

		char2type['{'] = kCharTypeBracket;
		char2type['('] = kCharTypeBracket;
		char2type['['] = kCharTypeBracket;

		char2type['}'] = kCharTypeBracketClose;
		char2type[')'] = kCharTypeBracketClose;
		char2type[']'] = kCharTypeBracketClose;

		char2type['*'] = kCharTypeSymbol;
		char2type['/'] = kCharTypeSymbol;
		char2type['-'] = kCharTypeSymbol;
		char2type['+'] = kCharTypeSymbol;
		char2type['%'] = kCharTypeSymbol;

		char2type['|'] = kCharTypeSymbol;
		char2type['^'] = kCharTypeSymbol;
		char2type['&'] = kCharTypeSymbol;

		char2type['='] = kCharTypeSymbol;
		char2type['!'] = kCharTypeSymbol;
		char2type['<'] = kCharTypeSymbol;
		char2type['>'] = kCharTypeSymbol;
		char2type['?'] = kCharTypeSymbol;

		char2type['.'] = kCharTypeSymbol;
		char2type['@'] = kCharTypeSymbol;

		char2type[':'] = kCharTypeSymbol;
		char2type[';'] = kCharTypeSymbol;
		char2type[','] = kCharTypeSymbol;

		char2type['~'] = kCharTypeSymbol;

		//char2type['\\'] = kCharTypeSymbol;

		REFLEX_LOOP_PTR(char2type + 'a', c, 26) *c = kCharTypeWord;
		REFLEX_LOOP_PTR(char2type + 'A', c, 26) *c = kCharTypeWord;
		char2type['_'] = kCharTypeWord;

		char2type['0'] = kCharTypeNumber;
		char2type['1'] = kCharTypeNumber;
		char2type['2'] = kCharTypeNumber;
		char2type['3'] = kCharTypeNumber;
		char2type['4'] = kCharTypeNumber;
		char2type['5'] = kCharTypeNumber;
		char2type['6'] = kCharTypeNumber;
		char2type['7'] = kCharTypeNumber;
		char2type['8'] = kCharTypeNumber;
		char2type['9'] = kCharTypeNumber;

		char2type['$'] = kCharTypeHex;

		char2type[char(39)] = kCharTypeSingleQuotedString;
		char2type['"'] = kCharTypeDoubleQuotedString;
	}

	NullTokeniserImpl()
	{
		Init(kChar2Type);
	}

	virtual TRef <Tokeniser> OnPush(UInt line, const char & bracket) override { return this; }

	virtual void OnPop(UInt line, const char & bracket, Tokeniser & child) override {}

	virtual void OnComment(UInt line, const CString::View & comment) override {}

	virtual void OnValue(UInt line, ValueType type, const CString::View & word) override {}

	virtual void OnSymbol(UInt line, const char & symbol) override {}
};

REFLEX_INLINE bool IsNotEOL(char c)
{
	return c != kLF && c != kCR;
}

REFLEX_INLINE bool IsNumberString(char c, Tokeniser::ValueType & type)
{
	if (kChar2Type[c] == kCharTypeNumber)
	{
		return true;
	}
	else if (c == '.')
	{
		type = Tokeniser::kValueTypeFloat;

		return true;
	}

	return false;
}

void ParseScope(Tokeniser * parent, Tokeniser & callbacks, UInt & line, char symbol, CString::View & buffer);

inline const CString::View kEndComment = "*/";

NullTokeniserImpl g_null_tokeniser;

void ParseScope(Tokeniser * parent, Tokeniser & callbacks, UInt & line, char symbol, CString::View & buffer)
{
	const auto char2type = kChar2Type;

	while (buffer)
	{
		auto c = buffer[0];

		if (c == '/')
		{
			if (buffer.size > 1)
			{
				auto next = buffer.data[1];

				if (next == '/')
				{
					buffer = Nudge(buffer, 2);

					callbacks.OnComment(line, Trim(ExtractWhile<IsNotEOL>(buffer)));

					continue;
				}
				else if (next == '*')
				{
					buffer = Nudge(buffer, 2);

					if (auto end = Search(buffer, kEndComment))
					{
						auto comment = Splice(buffer, end.value).a;

						while (auto idx = SearchCharacter(comment, kLF))
						{
							comment = Splice(comment, idx.value + 1).b;

							line++;
						}

						buffer = Nudge(buffer, end.value + 2);

						continue;
					}

					TokeniseFail(line, kErrorParseError);
				}
			}
		}

		switch (char2type[c])
		{
		case kCharTypeWhiteSpace:
			PreInc(buffer);
			break;

		case kCharTypeBracket:
			{
				auto child = AutoRelease(callbacks.OnPush(line, *buffer.data));
				ParseScope(&callbacks, child, line, c, PreInc(buffer));
			}
			break;

		case kCharTypeBracketClose:
			switch (Reinterpret<UInt16>(MakeTuple(symbol,c)))
			{
			case ID32("{}"):
			case ID32("[]"):
			case ID32("()"):
				parent->OnPop(line, *buffer.data, callbacks);
				buffer = Nudge(buffer);
				return;

			default:
				TokeniseFail(line, kErrorScopeMismatch);
				break;
			}
			break;

		case kCharTypeSymbol:
			callbacks.OnSymbol(line, *buffer.data);
			buffer = Nudge(buffer);
			break;

		case kCharTypeWord:
			{
				auto value = ExtractWhile<IsAlphaNumericCharacter>(buffer);
				callbacks.OnValue(line, Tokeniser::kValueTypeWord, value);
			}
			break;

		case kCharTypeNumber:
			{
				auto type = Tokeniser::kValueTypeInt;
				auto value = ExtractWhile<IsNumberString>(buffer, type);
				callbacks.OnValue(line, type, value);
			}
			break;

		case kCharTypeHex:
			callbacks.OnValue(line, Tokeniser::kValueTypeHex, ExtractWhile<IsAlphaNumericCharacter>(PreInc(buffer)));
			break;

		case kCharTypeSingleQuotedString:
			callbacks.OnValue(line, Tokeniser::kValueTypeSingleQuotedString, IterateString(PreInc(buffer), true));
			break;

		case kCharTypeDoubleQuotedString:
			callbacks.OnValue(line, Tokeniser::kValueTypeDoubleQuotedString, IterateString(PreInc(buffer), false));
			break;

		case kCharTypeLF:
			line++;
			PreInc(buffer);
			break;

		case kCharTypeCR:
			line++;
			if (buffer.size > 1 && buffer[1] == kLF)
			{
				buffer.data += 2;
				buffer.size -= 2;
			}
			else
			{
				//PreInc(buffer);
				TokeniseFail(line - 1, kErrorUnsupportedLineEnding);
			}
			break;

		default:
			if (IsAlphaNumericCharacter(c))
			{
				TokeniseFail(line, kErrorParseError);
			}
			else
			{
				TokeniseFail(line, kErrorUnexpectedSymbol);
			}
			break;
		}
	}

	if (symbol != 0)
	{
		TokeniseFail(line, kErrorScopeMismatch);
	}
}

REFLEX_END_INTERNAL

REFLEX_NOINLINE Reflex::Idx Reflex::Data::Detail::SearchCharacter(const CString::View & text, char character)
{
	return Search(text, character);
}

Reflex::Data::Detail::CharType Reflex::Data::Detail::kChar2Type[128];

void Reflex::Data::Detail::Tokeniser::Tokenise(Tokeniser & tokeniser, const CString::View & input, UInt & line)
{
	line = 0;

	CString::View buffer = input;

	ParseScope(0, tokeniser, line, 0, buffer);
}

void Reflex::Data::Detail::TokeniseFail(UInt line, const CString::View & text)
{
	throw ParseError({ line, text });
}

Reflex::Data::Detail::Tokeniser & Reflex::Data::Detail::Tokeniser::null = Reflex::Data::Detail::g_null_tokeniser;

#pragma once

#include "[require].h"



//
//

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

struct Token : public Reflex::Node <Token>
{
	static Token & null;

	enum Type : UInt8
	{
		kTypeNull,

		kTypeKeyword,
		kTypeSymbol,

		kTypeBracket,	// {}, (), [] (not <>)

		kTypeWord,
		kTypeInt,
		kTypeFloat,
		kTypeHex,

		kTypeSingleQuotedString,
		kTypeDoubleQuotedString,

		kTypeComment,

		kNumTokenType,
	};


	Token();

	Token(Token & parent, UInt linenumber, Type type);

	Token(Token & parent, UInt linenumber, Type type, const CString::View & value, Key32 hash);


	UInt linenumber;

	Type type;

	CString::View value;

	Key32 hash;


	using Reflex::Node<Token>::Attach;

	using Reflex::Node<Token>::InsertBefore;

	using Reflex::Node<Token>::InsertAfter;

	using Reflex::Node<Token>::Detach;

};

Reference <Token> Tokenise(const CString::View & input);

REFLEX_END_INTERNAL

REFLEX_SET_TRAIT(VM::Detail::Token, IsSingleThreadExclusive);




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

REFLEX_STATIC_ASSERT(sizeof(Token) <= 128);

struct Root : public Token
{
	Root()
		: allocator(CreateAllocator(K32("FastBlock128"), REFLEX_NULL(Object)))
	{
		allocator.RetainSt();
	}

	~Root()
	{
		Clear();

		allocator.ReleaseSt();
	}

	Reflex::Allocator & allocator;
};

struct TransitionalCallbacks : public Data::Detail::Tokeniser
{
	TransitionalCallbacks(Reflex::Allocator & allocator, Token & scope);

	virtual void OnComment(UInt line, const CString::View & comment) override {}

	virtual TRef <Tokeniser> OnPush(UInt line, const char & symbol) override;

	virtual void OnPop(UInt line, const char & symbol, Tokeniser & child) override;

	virtual void OnValue(UInt line, ValueType type, const CString::View & word) override;

	virtual void OnSymbol(UInt line, const char & symbol) override;


	Reflex::Allocator & allocator;

	Array <Token*> m_stack;
};

Reference <Token> Tokenise(const CString::View & input)
{
	auto root = REFLEX_CREATE(Root);

	TransitionalCallbacks callbacks(root->allocator, *root);

	Reference <Token> token = root;

	UInt line;

	TransitionalCallbacks::Tokenise(callbacks, input, line);

	return token;
}

TransitionalCallbacks::TransitionalCallbacks(Reflex::Allocator & allocator, Token & scope)
	: allocator(allocator)
{
	m_stack.Push(&scope);
}

REFLEX_INLINE Token::Token()
	: linenumber(0),
	type(kTypeNull)
{
}

REFLEX_INLINE Token::Token(Token & parent, UInt linenumber, Type type)
	: linenumber(linenumber),
	type(type)
{
	Attach(parent);
}

REFLEX_INLINE Token::Token(Token & parent, UInt linenumber, Type type, const CString::View & value, Key32 hash)
	: linenumber(linenumber),
	type(type),
	value(value),
	hash(hash)
{
	Attach(parent);
}

REFLEX_END_INTERNAL





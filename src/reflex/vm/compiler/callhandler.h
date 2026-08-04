#pragma once

#include "expressionhandler.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

struct CompilerImpl::CallHandler
{
	struct Input;



	//helpers

	static bool ParseTArgs(Source & src, Itr & itr, Input & input);



	//TODO move to ExpressionnHandler

	//static TypeRef DispatchCall(Source & src, Scope & scope, Itr & itr, const Pair <CString::View> & symbol);

	//static TypeRef DispatchMethod(Source & src, Scope & scope, Itr & itr, const Pair <CString::View> & symbol, const Variable & caller);

	static TypeRef DispatchCall(Source & src, Scope & scope, Itr & itr, const Pair <CString::View> & symbol, Input & input);	//pre set input with caller if method


	//return of 0 means failed

	static TypeRef Resolve(const CString::View & ns, Key32 symbol, Input & input);

	static TypeRef ResolveOperator(const ArrayView <Key32> & namespaces, Key32 op, Input & input);



	//secondary

	static Array <const Function*> Lookup(const CString::View & ns, Key32 symbol, Input & input);



	//helpers

	static bool IsAssociativeArguments(Itr itr);

	static void BuildRegularArguments(Input & input, Itr & itr, UInt n = kMaxUInt32);

	static void BuildAssociativeArguments(Input & input, Itr & itr, UInt n = kMaxUInt32);

	static Input BuildNamedConstructorInput(Source & src, Scope & scope, Itr & itr, TypeRef type);


	static Array <Key32> GetCallerNamespaces(CompilerImpl::Scope & scope, TypeRef caller);


private:

	struct TemplateFunctionsAccessor;

	struct FunctionsAccessor;

	struct Private;


	static void InstantiateTemplates(const Array <const State::TemplateFunction*> & candidates, Input & input);

	static TypeRef CallOverload(const Array <const Function*> & candidates, Input & input);

};

constexpr UInt64 MakeDelimiter(UInt32 symbol, Token::Type type)
{
	return symbol | (UInt64(type) << 32);
}

struct CompilerImpl::CallHandler::Input
{
	static constexpr UInt64 kDelimiterComma = MakeDelimiter(kSymbolComma, Token::kTypeSymbol);

	static constexpr UInt64 kDelimiterColon = MakeDelimiter(kSymbolColon, Token::kTypeSymbol);
	
	static constexpr UInt64 kDelimiterSemiColon = MakeDelimiter(kSymbolSemiColon, Token::kTypeSymbol);

	struct Slot
	{
		Variable variable;
		Itr itr;
		Instructions instructions;
	};

	struct Iterator;


	Input(Source & src, Scope & scope);

	Input(Input && t);

	void ClearTArgs()
	{
		m_targs.Clear();

		m_argshash.a = kHashSeed;
	}

	void AddTArg(const Argument & targ) { m_targs.Push(targ); Reflex::Detail::IncrementHash(m_argshash.a, targ.type->type_id); }



	void ClearArgs()
	{
		m_slots.Clear();

		m_argshash.b = kHashSeed;
	}

	void Add(const Variable & variable);		//use when already on stack

	void AddCaller(const Variable & object_variable) 
	{ 
		REFLEX_ASSERT(!m_slots);
		REFLEX_ASSERT(object_variable.type);

		if (object_variable.type)
		{ 
			REFLEX_ASSERT(!m_slots); 
			
			m_method = true; 
			
			Add(object_variable);
		}
	}

	Key32 Add(Itr & itr, const ArrayView <UInt64> & delimiters, bool consume_delimiter);	//returns delimiter type


	//helper -> move

	const UInt64 & GetArgsHash() const { return Reinterpret<UInt64>(m_argshash); }

	const Array <Argument> & GetTArgs() const { return m_targs; }

	Array <Slot> & GetSlots() { return m_slots; }

	const Array <Slot> & GetSlots() const { return m_slots; }


	bool IsMethod() const { return m_method; }


	operator bool() const { return True(m_slots); }



	Source & src;

	Scope & scope;


	decltype (&ExpressionHandler::CanCast) cancastfn;



private:


	bool m_method = false;

	Array <Argument> m_targs;

	Array <Slot> m_slots;	//a true: type on stack, false: code Itr

	Pair <UInt32> m_argshash;
};

inline void CompilerImpl::CallHandler::Input::Add(const Variable & variable)
{
	REFLEX_ASSERT(variable.type);

	//REFLEX_ASSERT(variable.type->IsObject() ? variable.location != Location::kTemporary : true);

	m_slots.Push({ variable, 0, { scope.allocator } });

	Reflex::Detail::IncrementHash(m_argshash.b, variable.type->type_id);
}

REFLEX_END_INTERNAL


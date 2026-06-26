#pragma once

#include "compilerimpl.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

struct CompilerImpl::FunctionHandler
{
	//primary

	static ScriptFunction * ParseFunctionDeclaration(Source & src, Itr & itr, Scope & scope, Key32 ns);


	static ScriptFunction & AssumeConstructor(Source & src, Itr & itr, Scope & scope, TypeRef type);	//move to DispatchClass

	static ScriptFunction * ParseMethod(Source & src, Itr & itr, Scope & scope, TypeRef type);	//move to DispatchClass



	//secondary

	struct Body;

	static Array <Argument> & AssumeArguments(Source & src, Itr & itr, Scope & scope);

	static Array <Argument> ParseTArgs(Source & src, Itr & itr, Scope & scope);

	static ScriptFunction & Retrieve(Source & src, Scope & scope, Key32 ns, const CString::View & tname, const ArrayView <Argument> & targs, const Argument & rtn, const ArrayView <Argument> & args);

	//static void AssumeBody(Source & src, Itr & itr, Scope & parent, ScriptFunction & fn);

	static bool ParseBody(Source & src, Itr & itr, Scope & parent, ScriptFunction & fn, const ArrayView < Pair <Symbol,Variable> > & captures = {});



private:

	static ScriptFunction * RetrieveExisting(Source & src, Scope & scope, Symbol symbol, UInt64 args_hash, const ArrayView <Argument> & targs, const ArrayView <Argument> & args);

	static Array <Argument> & ParseArguments(Source & src, Itr & itr, Scope & scope);

};

struct CompilerImpl::FunctionHandler::Body : public Scope
{
	Body(const Source & src, Scope & parent, ScriptFunction & fn);

	void DispatchReturn(Source & src, Itr & itr, Scope & subscope);

	void Finish();	//cant use constructor because of exception

	const Source & src;

	ScriptFunction & fn;


private:

	virtual bool OnHandleExit(Key32 keyword, Source & src, Itr & itr) override;

	Array < Pair <Symbol,Variable> > m_variables;

};

REFLEX_END_INTERNAL;

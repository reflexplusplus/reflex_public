#include "functionhandler.h"
#include "expressionhandler.h"




//
//


REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

ScriptFunction * CompilerImpl::FunctionHandler::ParseFunctionDeclaration(Source & src, Itr & itr, Scope & scope, Key32 ns)
{
	//need to diff between regular inline fn def and IMPL
	//need to make regular one is easy to detect -> TYPE + WORD + ()
	//impl on the other hand takes a type or namespace:
	// impl RTN_TYPE NS::NS::name @(TARGS)(args...)
	// method TYPE RTN_TYPE name @(TARGS)(args...)

	LOCAL(ScriptFunction&, Dispatch)(Source & src, Itr & itr, Scope & scope, Key32 ns, const CString::View & name, const Array <Argument> & targs, TypeRef rtn_t, const Array <Argument> & args)
	{
		auto & fn = Retrieve(src, scope, ns, name, targs, rtn_t, args);

		if (!ParseBody(src, itr, scope, fn)) ExpectSemiColon(src, itr);

		return fn;
	}
	END

	ParserRestorePoint store(itr);

	if (auto rtn_t = scope.ParseTypeOrAuto(src, itr))
	{
		auto & state = scope.state;

		if (state.IsAuto(rtn_t)) rtn_t = 0;

		//FULL soltuion is to use GetNamespacedSymbol in addition to current mechasism, for relative resolution
		//temp solution use either FULL path or local ns
		//in addition to not resolving within namespaced, this will also fail with :: for glob

		//auto RetrieveNamespacedProxy = [](Source & src, Scope & scope, const Pair <CString::View> & symbol, const ArrayView <Argument> & targs, const Argument & rtn, const ArrayView <Argument> & args) -> ScriptFunction &
		//{
		//	if (symbol.a)
		//	{
		//		return *Assume(RetrieveExisting(src, scope, { symbol.a, symbol.b }, HashArgs(targs, args), targs, args), SyntaxError(src, "namespaced function must be pre-declared"));
		//	}
		//	else
		//	{
		//		return Retrieve(src, scope, scope.GetNamespace(), symbol.b, targs, rtn, args);
		//	}
		//};

		auto name = ParseNamespacedSymbol(src, itr);

		if (name.b)
		{
			auto targs = ParseTArgs(src, itr, scope);

			if (auto brackets = BracketScope(src, itr, kBracketRound))
			{
				auto & args = AssumeArguments(src, brackets.inner, scope);

				if (name.a)
				{
					ns = Join(state.GetStaticString(ns, true), name.a);

					SubScope subscope(scope, ns);

					return &Dispatch::Call(src, itr, subscope, ns, name.b, targs, rtn_t, args);
				}
				else
				{
					return &Dispatch::Call(src, itr, scope, ns, name.b, targs, rtn_t, args);
				}
			}
		}
		else
		{
			SyntaxError::Throw(src);
		}
	}

	store.Rollback();

	return 0;
}

ScriptFunction & CompilerImpl::FunctionHandler::AssumeConstructor(Source & src, Itr & itr, Scope & scope, TypeRef type)
{
	Itr args = Assume(BracketScope(src, itr, kBracketRound), SyntaxError(src, kErrorExpectedBrackets)).inner;

	auto & arguments = AssumeArguments(src, args, scope);

	auto & fn = Retrieve(src, scope, type->symbol.a, Compiler::opCreate, { type }, type, arguments);

	if (!ParseBody(src, itr, scope, fn))
	{
		ExpectSemiColon(src, itr);
	}

	return fn;
}

ScriptFunction * CompilerImpl::FunctionHandler::ParseMethod(Source & src, Itr & itr, Scope & scope, TypeRef type)
{
	ParserRestorePoint restore(itr);

	if (auto return_t = scope.ParseType(src, itr))
	{
		//auto & nametoken = *Assume(Traverse<Token::kTypeWord>(src, itr), SyntaxError(src, kErrorExpectedName));

		if (auto pnametoken = Traverse<Token::kTypeWord>(src, itr))
		{
			auto & nametoken = *pnametoken;

			if (auto args = EnterBrackets(src, itr, kBracketRound))
			{
				auto & arguments = AssumeArguments(src, args.inner, scope);

				arguments.Insert(0, Argument(type, false, K32("this")));

				auto & fn = Retrieve(src, scope, type->symbol.a, nametoken.value, {}, return_t, arguments);

				if (!ParseBody(src, itr, scope, fn))
				{
					ExpectSemiColon(src, itr);
				}

				return &fn;
			}
		}
	}

	restore.Rollback();

	return 0;
}

Array <Argument> CompilerImpl::FunctionHandler::ParseTArgs(Source & src, Itr & itr, Scope & scope)
{
	struct TArgs : public Array <Argument> { using Array<Argument>::Array; void AddTArg(TypeRef type) { Push(type); } } targs(scope.allocator);

	scope.ParseTArgs(src, itr, targs);

	return std::move(targs);
}

Array <Argument> & CompilerImpl::FunctionHandler::AssumeArguments(Source & src, Itr & itr, Scope & scope)
{
	auto & state = scope.state;

	auto & rtn = state.arguments_cache;

	rtn.Clear();

	while (itr)
	{
		auto & argument = rtn.Push();

		argument.byref = false;

		argument.type = ExpectType(src, itr, scope);

		if (itr)
		{
			if (Traverse<Token::kTypeSymbol>(src, itr, kSymbolComma))
			{
				continue;
			}
			else
			{
				auto name = Assume(Traverse<Token::kTypeWord>(src, itr), SyntaxError(src, kErrorExpectedName))->value;

				argument.name = name;

				if (itr) Assume(Traverse<Token::kTypeSymbol>(src, itr, kSymbolComma), SyntaxError(src));
			}
		}
	}

	return rtn;
}

bool CompilerImpl::FunctionHandler::ParseBody(Source & src, Itr & brackets, Scope & scope, ScriptFunction & fn, const ArrayView < Pair <Symbol,Variable> > & captures)
{
	constexpr auto ParseFunctionBodyScope = [](Source & src, Itr & code, Scope & global, ScriptFunction & fn)
	{
		Body fnscope(src, global, fn);

		ParseScope(src, code, fnscope);

		fnscope.Finish();
	};

	auto spec = BracketScope(src, brackets, kBracketRound);

	if (auto code = BracketScope(src, brackets, kBracketBrace))
	{
		Assume(!fn.instructions, SyntaxError(src, kErrorDuplicateSymbol));

		if (spec)
		{
			auto & state = scope.state;

			Assume(TraverseWord(src, spec.inner, K32("mt")), SyntaxError(src));

			fn.flags2 |= Function::kFlagsMt;

			auto context_flags = state.bindings->context_flags;

			auto bindings = REFLEX_CREATE(Bindings, context_flags < 16 ? context_flags << 4 : context_flags);

			auto substate = REFLEX_CREATE(StateImpl, *bindings, REFLEX_NULL(Object));

			substate->m_compiler = state.m_compiler;

			while (spec.inner)
			{
				Assume(spec.inner->type == Token::kTypeDoubleQuotedString, SyntaxError(src));

				for (auto & i : Module::range)
				{
					if (i.id == spec.inner->hash)
					{
						substate->Instantiate(i);
					}
				}

				Inc(src, spec.inner);

				Traverse<Token::kTypeSymbol>(src, spec.inner, kSymbolComma);
			}

			//auto root = scope.state.root;

			struct MtScope : public Scope
			{
				MtScope(StateImpl & state)
					: Scope(state, true, false, 0, layout, variables, instructions, kNullKey),
					variables(allocator)
				{
					m_usings.Push(kNullKey);
				}

				virtual bool OnHandleExit(Key32 keyword, Source & src, Itr & itr) override
				{
					return Scope::OnHandleExit(keyword, src, itr);
				}

				LayoutTemplate layout;

				Array < Pair <Symbol,Variable> > variables;

				Instructions instructions;
			}

			mtscope(*substate);

			//for (auto & i : captures)
			//{
			//	auto & arg = fn.args.Push(i.b.type);
			//
			//	fn.arguments_size += arg.type->size;
			//}

			ParseFunctionBodyScope(src, code.inner, mtscope, fn);

			for (auto & i : substate->scriptfunction_layouts) state.scriptfunction_layouts.Insert(i.key, i.value);

			//scope.state.root = root;
		}
		else
		{
			ParseFunctionBodyScope(src, code.inner, scope, fn);
		}
		return true;
	}

	return false;
}

CompilerImpl::FunctionHandler::Body::Body(const Source & src, Scope & parent, ScriptFunction & fn)
	: Scope(parent.state, false, false, &parent, parent.state.scriptfunction_layouts.Acquire(&fn), m_variables, fn.instructions, parent.state.AcquireAnonSymbol()),
	src(src),
	fn(fn),
	m_variables(allocator)
{
	instructions.Allocate(16);

	//arguments will be on the stack before stack frame of function.  but take care of other data stored b4 stackframe, currently
	//reserved
	//Instruction* => this is current program pointer, is added by call and callfnobject
	//StackFrameLayout*	=> this is added by PushStackFrame, it is needed for abort stack frame unwind

	Int pre_stackframe_data_size = sizeof(Detail::CurrentPositionStore) + sizeof(Layout*);

	Int argument_offset = -Int(fn.arguments_size + pre_stackframe_data_size);

	REFLEX_ASSERT(argument_offset == GetArgumentsLocation(fn));

	Int bytes = 0;

	for (auto & i : fn.args)
	{
		Symbol symbol = { GetNamespace(), i.name };

		Assume(!SearchVariable(symbol), SyntaxError(src, kErrorDuplicateSymbol));

		variables.Push({ symbol, { i.type, Int16(argument_offset + bytes), Location::kLocal, i.type->IsObject()}});

		bytes += i.type->size;
	}
}

void CompilerImpl::FunctionHandler::Body::Finish()
{
	if (fn.rtn.type)
	{
		auto last = instructions ? instructions.GetLast() : Instruction();

		if ((last.opcode == OPCODE(Return) || last.opcode == OPCODE(ReturnObject)))
		{
		}
		else
		{
			Assume(fn.rtn.type == bindings.void_t, SyntaxError(src, kErrorExpectedReturn));

			AddReturn(instructions, src, fn);
		}
	}
	else //auto -> void
	{
		fn.rtn.type = bindings.void_t;

		AddReturn(instructions, src, fn);
	}

	REFLEX_ASSERT(!fn.flags2);

	fn.flags2 |= stackframelayout.objects ? 0 : ScriptFunction::kFlagsNoObjects;
}

void CompilerImpl::FunctionHandler::Body::DispatchReturn(Source & src, Itr & itr, Scope & subscope)
{
	bool contains = false;

	auto scopeitr = &subscope;

	while (scopeitr)
	{
		contains = contains || scopeitr == this;

		scopeitr = scopeitr->GetPrev();
	}

	REFLEX_ASSERT(contains);

	auto & state = subscope.state;

	auto return_t = itr ? ExpressionHandler::Parse(src, itr, *state.current, ExpressionHandler::kContextTemporary, fn.rtn.type) : bindings.void_t;

	if (fn.rtn.type)
	{
		Assume(return_t == fn.rtn.type || state.InheritsFrom(return_t, fn.rtn.type), TypeError(src));
	}
	else
	{
		fn.rtn.type = return_t;
	}

	//ExpectSemiColon(src, itr);

	if (instructions)
	{
		auto & last = instructions.GetLast();

		if ((last.opcode == OPCODE(Return) || last.opcode == OPCODE(ReturnObject)) && last.param64 == ToUIntNative(&fn)) return;
	}

	AddReturn(instructions, src, fn);
}

bool CompilerImpl::FunctionHandler::Body::OnHandleExit(Key32 keyword, Source & src, Itr & itr)
{
	if (keyword == kreturn)
	{
		DispatchReturn(src, itr, *this);

		return true;
	}
	else if (keyword == kexit)
	{
		AddAbort(instructions, src);

		return true;
	}

	return false;
}

ScriptFunction & CompilerImpl::FunctionHandler::Retrieve(Source & src, Scope & scope, Key32 ns, const CString::View & tname, const ArrayView <Argument> & targs, const Argument & rtn, const ArrayView <Argument> & args)
{
	auto & state = scope.state;

	Symbol symbol = {ns, tname};

	auto args_hash = HashArgs(targs, args);

	if (auto fn = RetrieveExisting(src, scope, symbol, args_hash, targs, args)) return *fn;

	auto target = Cast<ProgramImpl>(state.m_target);

	auto & fn = target->functions.Insert(ToUInt64(symbol));

	fn.symbol = symbol;	//.Set(m_localns, name);

	fn.name = AcquireStaticString(state, tname);

	fn.type = Function::kTypeScript;

	//fn.subtype = 0;

	fn.rtn = rtn;

	fn.targs = targs;

	fn.args = args;

	fn.args_hash = args_hash;

	fn.arguments_size = 0;

	for (auto & i : args) fn.arguments_size += i.type->size;

	return fn;
}

ScriptFunction * CompilerImpl::FunctionHandler::RetrieveExisting(Source & src, Scope & scope, Symbol symbol, UInt64 args_hash, const ArrayView <Argument> & targs, const ArrayView <Argument> & args)
{
	for (auto & i : scope.state.m_target->GetScriptFunctions(symbol))
	{
		auto & fn = i.value;

		if (fn.args_hash == args_hash)
		{
			auto pargs = args.data;

			auto ptargs = targs.data;

			if (bool match = fn.args.GetSize() == args.size && fn.targs.GetSize() == targs.size)
			{
				REFLEX_FOREACH(arg, fn.args) match = match && arg.type == (pargs++)->type;

				REFLEX_FOREACH(targ, fn.targs) match = match && targ.type == (ptargs++)->type;

				if (match) return RemoveConst(&fn);
			}
		}
	}

	return 0;
}

REFLEX_END_INTERNAL

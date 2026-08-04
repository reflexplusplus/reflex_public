#include "callhandler.h"
#include "expressionhandler.h"

//for vscode
#include "compilerimpl.h"
#include "scope.h"
#include "expressionhandler.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

UInt GetSignatureType(UInt flags)
{
	flags >>= 2;

	flags &= 3;

	REFLEX_ASSERT(flags < 4);

	return flags;
}

UInt GetSignatureType(const Function & function)
{
	return GetSignatureType(function.flags2);
}

const Argument & GetArg(const ArrayView <Argument> & args, UInt idx)
{
	return args[idx];
}

const Argument & GetVaradicArg(const ArrayView <Argument> & args, UInt idx)
{
	REFLEX_ASSERT(args);

	auto [head, tail] = ReverseSplice(args, 1);

	if (idx < head.size)
	{
		return args[idx];
	}

	return tail.GetFirst();
}

const Argument & GetVaradicAssociativeArg(const ArrayView <Argument> & args, UInt idx)
{
	REFLEX_ASSERT(args);

	return args[idx % 2];
}

struct CompilerImpl::CallHandler::Private
{
	static inline const decltype (&GetArg) kArgGetters[] =
	{
		&GetArg,
		&GetVaradicArg,
		[](const ArrayView <Argument> & args, UInt idx) -> const Argument &
		{
			REFLEX_ASSERT(false);
			return args[0];
		},
		&GetVaradicAssociativeArg,
	};


	static TypeRef ApplyCall(CompilerImpl::CallHandler::Input & input, const Function & fn);
};

struct CompilerImpl::CallHandler::TemplateFunctionsAccessor
{
	typedef Key32 SymbolType;

	typedef Input & Param;	//NTarg

	typedef Array <const State::TemplateFunction*> Output;

	static bool Equal(UInt ntfnarg, UInt ninputargs)
	{
		return ntfnarg == ninputargs;
	}

	static bool GTE(UInt ntfnarg, UInt ninputargs)
	{
		return ninputargs >= ntfnarg;
	}

	static bool EqualAssociative(UInt ntfnarg, UInt ninputargs)
	{
		REFLEX_ASSERT(ntfnarg == 2);

		return (ninputargs & 1) == 0;
	}

	static inline const decltype(&TemplateFunctionsAccessor::Equal) kArgsMatch[4] = 
	{ 
		&TemplateFunctionsAccessor::Equal, 
		&TemplateFunctionsAccessor::GTE, 
		[](UInt ntfnarg, UInt ninputargs)
		{
			REFLEX_ASSERT(false);
			return false;
		},
		//&TemplateFunctionsAccessor::EqualAssociative 
		&TemplateFunctionsAccessor::EqualAssociative
	};

	static bool Get(Scope & scope, Symbol symbol, Input & input, Output & output)
	{
		auto narg = input.GetSlots().GetSize();

		auto ntarg = input.GetTArgs().GetSize();

		auto & symbolid = Reinterpret<UInt64>(symbol);

		for (auto & i : MakeRange(scope.state.m_template_functions, symbolid, symbolid + 1))
		{
			auto & tfn = i.value;

			if (ntarg == tfn.ntarg && kArgsMatch[GetSignatureType(tfn.flags)](tfn.arguments.GetSize(), narg))
			{
				output.Push(&tfn);
			}
		}

		return true;
	}
};

struct CompilerImpl::CallHandler::FunctionsAccessor
{
	typedef Key32 SymbolType;

	typedef Input & Param;	//targs, narg

	typedef Array <const Function*> Output;

	static bool Get(Scope & scope, Symbol symbol, Input & input, Output & output)
	{
		auto argshash = Reinterpret<Pair<UInt32>>(input.GetArgsHash());

		auto narg = input.GetSlots().GetSize();

		//auto ntarg = input.GetTArgs().GetSize();

		auto & bindings = scope.bindings;

		//priotise script to overload anything

		for (auto & i : scope.state.m_target->GetScriptFunctions(symbol))
		{
			auto & fn = i.value;

			auto & fn_argshash = Reinterpret<Pair<UInt32>>(fn.args_hash);

			if (fn_argshash.a == argshash.a && fn.args.GetSize() == narg)
			{
				if (fn_argshash.b == argshash.b)
				{
					output.Insert(0, &fn);

					return false;
				}
				else
				{
					output.Push(&fn);
				}
			}
		}

		for (auto & i : bindings.GetExternalFunctions(symbol))
		{
			auto & fn = i.value;

			auto & fn_argshash = Reinterpret<Pair<UInt32>>(fn.args_hash);

			if (fn_argshash.a == argshash.a)	//fn.targs.GetSize() == ntarg)
			{
				if (fn_argshash.b == argshash.b)		//exact args match
				{
					output.Insert(0, &fn);

					return false;
				}
				else if (fn.args.GetSize() == narg || (fn.flags2 & ExternalFunction::kFlagsVaradic))		//allow autocast
				{
					output.Push(&fn);
				}
			}
		}

		for (auto & i : bindings.GetIntrinsics(symbol))
		{
			auto & intrinsic = i.value;

			auto & fn_argshash = Reinterpret<Pair<UInt32>>(intrinsic.args_hash);

			if (fn_argshash.a == argshash.a && intrinsic.args.GetSize() == narg)			//check targs
			{
				if (fn_argshash.b == argshash.b)		//args exact match
				{
					output.Insert(0, &intrinsic);

					return false;
				}
				else		//allow autocast
				{
					output.Push(&intrinsic);
				}
			}
		}

		return true;
	}
};

void CompilerImpl::CallHandler::BuildRegularArguments(Input & self, Itr & itr, UInt n)
{
	REFLEX_ASSERT(n);

	const auto kComma = ToView(Input::kDelimiterComma);

	while (itr && n)
	{
		self.Add(itr, kComma, true);

		n--;
	}

	Assume(!itr, SyntaxError(self.src));
}

void CompilerImpl::CallHandler::BuildAssociativeArguments(Input & self, Itr & itr, UInt n)
{
	REFLEX_ASSERT(n);

	if (itr)
	{
		auto key = self.Add(itr, ToView(Input::kDelimiterColon), true);

		Assume(key == kSymbolColon, SyntaxError(self.src, kErrorExpectedVariable));

		auto comma = self.Add(itr, ToView(Input::kDelimiterComma), true);	//was comma, semicolon

		n--;

		while (comma == kSymbolComma && n)
		{
			Assume(Copy(itr), SyntaxError(self.src, kErrorExpectedVariable));	//expect key

			auto key = self.Add(itr, ToView(Input::kDelimiterColon), true);

			Assume(key == kSymbolColon, SyntaxError(self.src, kErrorExpectedVariable));

			comma = self.Add(itr, ToView(Input::kDelimiterComma), true);	//was comma, semicolon

			n--;
		}

		Assume(!itr, SyntaxError(self.src));
	}
}

bool CompilerImpl::CallHandler::IsAssociativeArguments(Itr t)
{
	while (t)
	{
		if (t->hash == kSymbolColon)
		{
			return true;
		}

		t = t->GetNext();
	}

	return false;
}

CompilerImpl::CallHandler::Input::Input(Source & src, Scope & scope)
	: src(src),
	scope(scope),
	m_argshash({ kHashSeed, kHashSeed }),
	m_slots(scope.allocator),
	m_targs(scope.allocator),
	cancastfn(&ExpressionHandler::CanCast)
{
	m_slots.Allocate(3);
}

CompilerImpl::CallHandler::Input::Input(Input && t)
	: src(t.src),
	scope(t.scope),
	m_argshash(std::move(t.m_argshash)),
	m_slots(std::move(t.m_slots)),
	m_targs(std::move(t.m_targs)),
	cancastfn(&ExpressionHandler::CanCast)
{
}

Key32 CompilerImpl::CallHandler::Input::Add(Itr & itr, const ArrayView <UInt64> & delimiters, bool consume_delimiter)
{
	REFLEX_ASSERT(itr);
	//TODO bug Consume doesnt work
	//try using itr after RenderScope::Parse for full Expression parse

	//PropertySet t;

	//t#a = []()
	//{
	//}

	//t#b = []()
	//{
	//};

	//constexpr auto Consume = [](Source & src, Itr & itr, Key32 & delimiter, const ArrayView <UInt64> & delimiters) -> Itr
	//{
	//	auto rtn = itr;

	//	while (itr)
	//	{
	//		auto symbol = MakeDelimiter(itr->hash, itr->type);

	//		if (Search(delimiters, symbol))
	//		{
	//			delimiter = itr->hash;

	//			return rtn;
	//		}
	//		else if (symbol == kDelimiterSemiColon)
	//		{
	//			return rtn;
	//		}
	//		else
	//		{
	//			itr = itr->GetNext();
	//		}
	//	}

	//	return rtn;
	//};

	//auto restore = itr;

	//auto start = Consume(src, itr, delimiter, delimiters);

	//Assume(Copy(start), SyntaxError(src, kErrorInternalError));

	auto & slot = m_slots.Push({ {}, itr, { scope.allocator } });

	ExpressionHandler::RenderScope render(scope, slot.instructions);

	try
	{
		//auto start = itr;

		auto result = render.Parse2(src, std::move(itr), 0);

		auto result_t = result.type;

		REFLEX_ASSERT(result_t /*&& result_t != scope.bindings.void_t*/);//should be handled by error

		slot.variable = result;

		Reflex::Detail::IncrementHash(m_argshash.b, result_t->type_id);
	}
	catch (const TypeError &)
	{
		slot.instructions.Clear();

		Reflex::Detail::IncrementHash(m_argshash.b, scope.bindings.void_t->type_id);
	}

	if (itr)
	{
		auto delimiter = itr->hash;

		if (Search(delimiters, MakeDelimiter(delimiter.value, itr->type)))
		{
			if (consume_delimiter)
			{
				Inc(src, itr);
			}

			return delimiter;
		}

		SyntaxError::Throw(src);
	}

	return {};
}

Array <Key32> CompilerImpl::CallHandler::GetCallerNamespaces(CompilerImpl::Scope & scope, TypeRef caller_t)
{
	REFLEX_ASSERT(caller_t);

	Array <Key32> usings(scope.GetUsings(), scope.allocator);

	auto types = scope.state.RetrieveInheritanceInfo(caller_t).b;

	for (auto & i : types)
	{
		if (!Search(usings, i)) usings.Push(i);
	}

	return usings;
}

bool CompilerImpl::CallHandler::ParseTArgs(Source & src, Itr & itr, Input & input)
{
	return input.scope.ParseTArgs(src, itr, input);
}

void CompilerImpl::CompilerImpl::CallHandler::InstantiateTemplates(const Array <const State::TemplateFunction*> & candidates, Input & input)
{
	INLINE(bool,GetNextArgType)(const State::TemplateFunction & tfn, Input & input, UInt idx, TypeRef & type)
	{
		if (idx < input.GetSlots().GetSize())
		{
			auto & args = tfn.arguments;

			switch (tfn.flags & Bits<false,false,true,true>::value)
			{
			case 0:
				REFLEX_ASSERT(idx < args.GetSize());
				type = args[idx].type;
				return true;

			case ExternalFunction::kFlagsVaradic:
				REFLEX_ASSERT(args);
				if (idx < args.GetSize() - 1)
				{ 
					type = args[idx].type;
				}
				else
				{
					type = args.GetLast().type;
				}
				return true;

			case (ExternalFunction::kFlagsVaradic | ExternalFunction::kFlagsAssociative):
				REFLEX_ASSERT(args);
				//TODO could make this support head args too
				type = args[idx % 2].type;
				return true;

			default:
				REFLEX_ASSERT(false)
				return false;
			}
		}

		return false;
	}
	END

	auto & targs = input.GetTArgs();

	auto & scope = input.scope;

	auto & state = scope.state;

	auto & allocator = scope.allocator;

	Array <Argument> args(allocator);

	for (auto & i : candidates)
	{
		auto & tfn = *i;

		REFLEX_ASSERT(targs.GetSize() == tfn.ntarg && (tfn.arguments.GetSize() == input.GetSlots().GetSize() || (tfn.flags & ExternalFunction::kFlagsVaradic)));

		UInt idx = 0;

		args.Clear();

		TypeRef arg_t;

		bool fast = true;

		while (GetNextArgType::Call(tfn, input, idx, arg_t))
		{
			auto & input_slot = input.GetSlots()[idx];

			auto input_t = input_slot.variable.type;

			if (arg_t)
			{
				if (input_t)
				{
					if (input_t == arg_t || ExpressionHandler::CanCast(input.src, scope, input_slot.variable, arg_t))
					{
						args.Push(arg_t);
					}
					else
					{
						goto Next;
					}
				}
				else
				{
					Instructions output(allocator);

					ExpressionHandler::RenderScope render(scope, output);

					auto result_t = render.Parse2(input.src, Copy(input_slot.itr), arg_t).type;

					if (result_t == arg_t)
					{
						args.Push(result_t);
					}
					else
					{
						goto Next;
					}
				}
			}
			else if (input_t)
			{
				args.Push(input_t);
			}
			else
			{
				Instructions output(allocator);

				try
				{
					ExpressionHandler::RenderScope render(scope, output);

					args.Push(render.Parse2(input.src, Copy(input_slot.itr), arg_t).type);
				}
				catch (const TypeError &)
				{
					//args.Push(0);

					//fast = false;

					goto Next;
				}
			}

			idx++;
		}

		if (fast)
		{
			auto hash = HashArgs(input.GetTArgs(), args);

			hash = Reflex::Detail::MergeHashes(hash, ToUInt64(tfn.symbol));

			auto & guard = state.instantiated_tfn_guard.Acquire(hash);

			if (!guard)
			{
				REFLEX_ASSERT(args.GetSize() == tfn.arguments.GetSize() || (tfn.flags & ExternalFunction::kFlagsVaradic));

				if (tfn.instantiate(state, tfn.symbol.a, tfn.name, targs, args))
				{
					guard = true;

					return;
				}
			}
		}
		else
		{
			REFLEX_ASSERT(args.GetSize() == tfn.arguments.GetSize() || (tfn.flags & ExternalFunction::kFlagsVaradic));

			if (tfn.instantiate(state, tfn.symbol.a, tfn.name, targs, args))
			{
				return;
			}
		}

		REFLEX_MARKER(Next);
	}
}

REFLEX_NOINLINE TypeRef CompilerImpl::CallHandler::CallOverload(const Array <const Function*> & candidates, Input & input)
{
	INLINE(bool,MatchArg)(const Argument & arg, const Input & input, UInt idx, UInt & castcount)
	{
		auto arg_t = arg.type;

		REFLEX_ASSERT(arg_t);	//should not be templated here anymore (?)

		auto & scope = input.scope;

		auto & slot = input.GetSlots()[idx];

		if (auto type = slot.variable.type)
		{
			return type == arg_t ? true : input.cancastfn(input.src, scope, slot.variable, arg_t, castcount);
		}
		else
		{
			Instructions output(scope.allocator);

			ExpressionHandler::RenderScope render(scope, output);

			auto result = render.Parse2(input.src, Copy(slot.itr), arg_t);

			return result.type == arg_t ? true : input.cancastfn(input.src, scope, result, arg_t, castcount);
		}
	}
	END

	//INLINE(bool,ParseArgumentsStandard)(const ArrayView <Argument> & args, const Input & input, UInt & castcount)
	//{
	//	REFLEX_ASSERT(args.b == input.GetSlots().GetSize());	//this is now done at lookup

	//	UInt idx = 0;

	//	for (auto & i : args)
	//	{
	//		if (!MatchArg::Call(i, input, idx++, castcount)) return false;
	//	}

	//	return true;
	//}
	//END

	//INLINE(bool,ParseArgumentsVaradicArray)(const ArrayView <Argument> & args, const Input & input, UInt & castcount)
	//{
	//	REFLEX_LOOP(idx, input.GetSlots().GetSize())
	//	{
	//		auto & arg = GetVaradicArg(args, idx);

	//		if (!MatchArg::Call(arg, input, idx++, castcount)) return false;
	//	}

	//	return true;
	//}
	//END

	//INLINE(bool, ParseArgumentsVaradicMap)(const ArrayView <Argument> & args, const Input & input, UInt & castcount)
	//{
	//	REFLEX_ASSERT(args);

	//	REFLEX_LOOP(idx, input.GetSlots().GetSize())
	//	{
	//		auto & arg = GetVaradicAssociativeArg(args, idx);

	//		if (!MatchArg::Call(arg, input, idx++, castcount)) return false;
	//	}

	//	return true;
	//}
	//END
	//	
	//constexpr decltype (&ParseArgumentsStandard::Call) kParsers[] =
	//{ 
	//	&ParseArgumentsStandard::Call, 
	//	&ParseArgumentsVaradicArray::Call,
	//	[](const ArrayView <Argument> & args, const Input & input, UInt & castcount)
	//	{
	//		REFLEX_ASSERT(false);
	//		return false;
	//	},
	//	&ParseArgumentsVaradicMap::Call,
	//};

	//if constexpr (REFLEX_DEBUG)
	//{
	//	for (auto & i : input.GetSlots())
	//	{
	//		REFLEX_ASSERT(i.variable.type->IsObject() ? i.variable.location != Variable::kLocationTemporary : true);
	//	}
	//}

	auto & scope = input.scope;

	Sequence <UInt,const Function*> matches(scope.allocator);

	for (auto & i : candidates)
	{
		auto & fn = *i;

		REFLEX_ASSERT(fn.rtn.type && input.GetTArgs().GetSize() == fn.targs.GetSize());

		auto args = ToView(fn.args);

		UInt castcount = 0;

		auto getarg = Private::kArgGetters[GetSignatureType(fn)];

		REFLEX_LOOP(idx, input.GetSlots().GetSize())
		{
			auto & arg = getarg(args, idx);

			if (!MatchArg::Call(arg, input, idx++, castcount)) goto Skip;
		}

		if (castcount)
		{
			matches.Insert(castcount, &fn);
		}
		else
		{
			return Private::ApplyCall(input, fn);
		}

		REFLEX_MARKER(Skip);
	}

	if (matches)
	{
		auto & match = matches.GetFirst().value;

		if (matches.GetSize() > 1)
		{
			if (matches.GetFirst().key < matches[1].key) return Private::ApplyCall(input, *match);
		}
		else
		{
			return Private::ApplyCall(input, *match);
		}

		TypeError::Throw(input.src, "ambiguous function call");
	}

	return 0;
}

TypeRef CompilerImpl::CallHandler::DispatchCall(Source & src, Scope & scope, Itr & itr, const Pair <CString::View> & pair, Input & input)
{
	INLINE(TypeRef,RenderInput)(Input & input, UInt idx, Instructions & output)
	{
		auto & slot = input.GetSlots()[idx];

		if (auto type = slot.variable.type)
		{
			output.Append(slot.instructions);

			return type;
		}
		else
		{
			ExpressionHandler::RenderScope render(input.scope, output);

			return render.Parse2(input.src, Copy(slot.itr), 0).type;
		}
	}
	END

	ParseTArgs(src, itr, input);

	Itr inner = Assume(BracketScope(src, itr, kBracketRound), SyntaxError(src)).inner;

	BuildRegularArguments(input, inner);

	if (auto result_t = Resolve(pair.a, pair.b, input))
	{
		return result_t;
	}

	auto u64 = Reinterpret<UInt64>(Symbol {pair.a, pair.b});

	if (u64 == Reinterpret<UInt64>(MakeTuple(kGlobal, kreinterpret)))
	{
		auto & targs = input.GetTArgs();

		Assume(targs.GetSize() == 1 && input.GetSlots().GetSize() == 1, SyntaxError(src, kErrorExpectedType));

		auto arg_t = targs.GetFirst().type;

		Assume(!arg_t->IsObject(), TypeError(src));

		auto result_t = RenderInput::Call(input, 0, scope.instructions);

		Assume(arg_t->size <= result_t->size, TypeError(src));

		return arg_t;
	}
	else if (u64 == Reinterpret<UInt64>(MakeTuple(kGlobal, ksizeof)))
	{
		auto type = ExpectType(src, itr, scope);

		AddPushConst(scope.instructions, src, Data::Pack(Int32(type->size)));

		return scope.bindings.int32_t;
	}

	scope.state.ThrowCustomError(src, "'[0]' matching function not found", { pair.b });

	return {};
}

CompilerImpl::CallHandler::FunctionsAccessor::Output CompilerImpl::CallHandler::Lookup(const CString::View & ns, Key32 symbol, Input & input)
{
	INLINE(FunctionsAccessor::Output, Apply)(const ArrayView <Key32> & namespaces, const CString::View & ns, Key32 symbol, Input & input)
	{
		if (auto tcandidates = GetSymbolsOfType<TemplateFunctionsAccessor>(input.scope, ns, symbol, namespaces, input))
		{
			InstantiateTemplates(tcandidates, input);
		}

		return GetSymbolsOfType<FunctionsAccessor>(input.scope, ns, symbol, namespaces, input);
	}
	END

	if (input.IsMethod())
	{
		auto & scope = input.scope;

		auto caller_t = input.GetSlots()[0].variable.type;

		auto usings = GetCallerNamespaces(scope, caller_t);

		return Apply::Call(usings, ns, symbol, input);
	}
	else
	{
		return Apply::Call(input.scope.GetUsings(), ns, symbol, input);
	}
}

TypeRef CompilerImpl::CallHandler::Resolve(const CString::View & ns, Key32 symbol, Input & input)
{
	if (auto candidates = Lookup(ns, symbol, input))
	{
		return CallOverload(candidates, input);
	}
	else
	{
		return 0;
	}
}

TypeRef CompilerImpl::CallHandler::ResolveOperator(const ArrayView <Key32> & usings, Key32 op, Input & input)
{
	auto & scope = input.scope;

	TemplateFunctionsAccessor::Output tcandidates(scope.allocator);

	FunctionsAccessor::Output candidates(scope.allocator);

	//auto & usings = scope.state.RetrieveInheritanceInfo(caller.type).b;

	Symbol symbol = { kNullKey, op };

	REFLEX_RFOREACH(ns, usings)
	{
		symbol.a = ns;

		tcandidates.Clear();

		TemplateFunctionsAccessor::Get(scope, symbol, input, tcandidates);

		InstantiateTemplates(tcandidates, input);

		FunctionsAccessor::Get(scope, symbol, input, candidates);
	}

	if (candidates)
	{
		return CallOverload(candidates, input);
	}
	else
	{
		return 0;
	}
}

TypeRef CompilerImpl::CallHandler::Private::ApplyCall(CompilerImpl::CallHandler::Input & input, const Function & fn)
{
	INLINE(Instruction&, GetLastInstruction)(Scope * scope, Instruction & fb)
	{
		if (auto & i = scope->instructions) return i.GetLast();

		while (scope->IsSub())
		{
			scope = scope->GetPrev();

			if (auto & i = scope->instructions) return i.GetLast();
		}

		return fb;
	}
	END

	LOCAL(void, ApplyArg)(CompilerImpl::CallHandler::Input & input, Input::Slot & slot, const Argument & arg)
	{
		auto & src = input.src;

		auto & scope = input.scope;

		auto & instructions = scope.instructions;

		auto arg_t = arg.type;

		instructions.Append(slot.instructions);

		auto variable = slot.variable;

		if (auto type = variable.type)
		{
			IS_RETAINED(variable);

			if (type != arg_t)
			{
				variable = ExpressionHandler::ExpectCast(src, scope, slot.variable, arg_t);

				if (variable.type->IsObject()) ExpressionHandler::ResolveIfObjectTemporary(src, scope, variable);
			}
		}
		else
		{
			variable = ExpressionHandler::ParseX(src, slot.itr, scope, ExpressionHandler::kContextTemporary, arg_t);
		}

		IS_RETAINED(variable);

		if (arg.byref)
		{
			Assume(!variable.is_const, SyntaxError(src, kErrorNonAssignableValue));

			Instruction error = { src.line, src.file, OPCODE(NumOpcode) };

			auto & last = GetLastInstruction::Call(&scope, error);

			switch (last.opcode)
			{
				//VAR BY REFERENCE SO OK
			case OPCODE(PushGlobalByAdr):
			case OPCODE(PushLocalByAdr):
			case OPCODE(PushMemberByAdr):
				break;

				//VAR BY VALUE -> CONVERT TO REF
			case OPCODE(PushGlobal):
			case OPCODE(PushLocal):
			case OPCODE(PushMember):
				//REFLEX_ASSERT(!type->IsObject());
				if (Reinterpret<Pair<UInt16>>(last.param32).b == variable.type->size)
				{
					last.opcode++;	//make ref
				}
				else
				{
					//NOT SURE IF THIS CAN STILL HAPPEN -> TEST

					//this can happen when for example AddRect(object, { stuff.a, stuff.b } );
					//CLEAN SOLUTION is to parse expression always creates temporaries, and these would be filtered out in optimiser
					//this requires quite a lot of of work to keep values being passed directly from temps 
					
					variable.location = Location::kTemporary;	//for debug

					ExpressionHandler::ResolveTemporary(src, scope, variable, true);
				}
				break;

				//TEMPORARY (other opcodes so must be push const, call or intrinsic)
			default:
				ExpressionHandler::ResolveTemporary(src, scope, variable, true);
				break;
			}		
		}
		//else if (variable.type->IsObject() && variable.location == Location::kTemporary)
		//{
		//	ExpressionHandler::ResolveTemporary(src, scope, variable, false);
		//}
	}
	END

	auto getarg = Private::kArgGetters[GetSignatureType(fn)];

	UInt idx = 0;

	auto & args = fn.args;

	REFLEX_FOREACH(slot, input.GetSlots())
	{
		ApplyArg::Call(input, slot, getarg(args, idx++));
	}

	return AddCall(input.scope.instructions, input.src, fn, input.GetSlots().GetSize());
};

CompilerImpl::CallHandler::Input CompilerImpl::CallHandler::BuildNamedConstructorInput(Source & src, Scope & scope, Itr & itr, TypeRef type)
{
	constexpr auto AddNamed = [](Input & input, Itr & itr)
	{
		Assume(Copy(itr), SyntaxError(input.src));

		Inc(input.src, itr);

		Assume(Copy(itr), SyntaxError(input.src));

		auto name = itr->hash;

		Inc(input.src, itr);

		Assume(itr->hash == kSymbolEqual, SyntaxError(input.src));

		Inc(input.src, itr);

		input.Add(itr, ToView(Input::kDelimiterComma), false);

		return name;
	};

	ParserRestorePoint restore(itr);

	Input input(src, scope);

	if (itr)
	{
		if (itr->hash == kSymbolDot)
		{
			for (auto & i : type->members)
			{
				if (AddNamed(input, itr) != i.a)
				{
					SyntaxError::Throw(input.src, kErrorMatchingConstructorNotFound);

					break;
				}

				if (itr)
				{
					Assume(itr->hash == kSymbolComma, SyntaxError(input.src));

					Inc(input.src, itr);
				}
			}

			if (itr) SyntaxError::Throw(input.src);
		}
		else
		{
			restore.Rollback();
		}
	}

	return input;
}

REFLEX_END_INTERNAL

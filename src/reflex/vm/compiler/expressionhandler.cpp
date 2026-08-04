#include "expressionhandler.h"
#include "callhandler.h"

//for vscode
#include "compilerimpl.h"
#include "scope.h"
#include "functionhandler.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

#define UNREACHABLE_X(ERROR_TYPE, ERROR_MSG) ERROR_TYPE::Throw(src, ERROR_MSG); return {};
#define UNREACHABLE UNREACHABLE_X(SyntaxError, kErrorInternalError)
#define DISPATCH(CASE, FN) case CASE: result = {FN}; break;
#define GOTO(MARKER, VAR, VALUE) VAR = {VALUE}; goto MARKER;

struct CompilerImpl::ExpressionHandler::Private
{
	inline static const Tuple <UInt,Key32,decltype(&CallHandler::BuildRegularArguments)> kCreateArrayInfo[] = {{1, kopCreateArray, &CallHandler::BuildRegularArguments }, {2, kopCreateMap, &CallHandler::BuildAssociativeArguments}};

	typedef Pair <Key32, TypeRef> VarRef;

	enum ContextX
	{
		kContextAssignment = kNumContext
	};

	enum Chain : UInt8
	{
		kChainPostOp,
		kChainRecurse,
	};


	static Variable Parse(Source & src, Itr & itr, Scope & scope, Context context, TypeRef lhs);


	static Variable DispatchBracketConstructor(Source & src, Itr & itr, Scope & scope, TypeRef lhs, UInt32 token);


	template <bool _32BIT> static TypeRef PushNumberLiteral(Scope & scope, Source & src, TypeRef type, const void * pvalue);

	template <bool FLOAT> static Variable DispatchNumberLiteral(Source & src, Itr & itr, Scope & scope);


	static Variable DispatchKeyword(Source & src, Itr & itr, Scope & scope, Context context, TypeRef lhs, UInt32 token);

	static TypeRef Dispatch_bind(Source & src, Itr & itr, Scope & scope, TypeRef lhs);


	static Variable DispatchWord(Source & src, Itr & itr, Scope & scope, Context context, TypeRef lhs, UInt32 token);

	static Variable DispatchWord_TypedVarDecl(Source & src, Itr & itr, Scope & scope, Context context, TypeRef variable_t);


	static TypeRef TraversePreOperator(Source & src, Itr & itr, Scope & scope, UInt32 token);


	static Variable ParseChain(Source & src, Scope & scope, Itr & itr, TypeRef lhs, Variable prev = {});

	static TypeRef TraversePostOperator(Source & src, Itr & itr, Scope & scope, Context context, UInt32 symbol, Variable first);


	static TypeRef AssumeConstructor(Source & src, Scope & scope, TypeRef type, const Token * inner);

	static TypeRef ApplyBind(Source & src, Scope & scope, Itr & itr, const Function & fn, Array <TypeRef> & captures);


	static TypeRef CallFunction(Source & src, Scope & scope, Itr & itr, const Pair <CString::View> & symbol)
	{
		CallHandler::Input input(src, scope);

		return CallHandler::DispatchCall(src, scope, itr, symbol, input);
	}

	static TypeRef CallOperatorMethod(Source & src, Scope & scope, Key32 symbol, CallHandler::Input & input)
	{
		auto & caller = input.GetSlots().GetFirst().variable;

		return CallHandler::ResolveOperator(CallHandler::GetCallerNamespaces(scope, caller.type), symbol, input);
	}


	REFLEX_INLINE static Variable MakeConst(TypeRef type)
	{
		return { type, 0, Location::kConst };
	}

	REFLEX_INLINE static Variable MakeTemporary(TypeRef type)
	{
		return { type, 0, Location::kTemporary };
	}
};

void CompilerImpl::ExpressionHandler::ResolveIfObjectTemporary(Source & src, Scope & scope, Variable & result)
{
	if (result.location == Location::kTemporary && result.type->IsObject())
	{
		result = ResolveTemporary(src, scope, result, false);
	}
}

Variable CompilerImpl::ExpressionHandler::ParseX(Source & src, Itr & itr, Scope & scope, Context context, TypeRef target_t)
{
	auto variable = Private::Parse(src, itr, scope, context, target_t);

	if (context != kNumContext) { IS_RETAINED(variable); }

	if (target_t && target_t != variable.type)
	{
		variable = ExpectCast(src, scope, variable, target_t);

		if (context != kNumContext /*&& target_t->IsObject()*/) ResolveIfObjectTemporary(src, scope, variable);
	}

	return variable;
}

TypeRef CompilerImpl::ExpressionHandler::Parse(Source & src, Itr & itr, Scope & scope, Context context, TypeRef lhs)
{
	return ParseX(src, itr, scope, context, lhs).type;
}

bool CompilerImpl::ExpressionHandler::CanCast(Source & src, Scope & scope, const Variable & variable, TypeRef target_t, UInt & castcount)
{
	auto from_t = variable.type;

	REFLEX_ASSERT(target_t);

	REFLEX_ASSERT(target_t != from_t);

	if (from_t == target_t)
	{
		return true;
	}
	else
	{
		auto & state = scope.state;

		auto & cache = state.castable_cache;

		Pair <UInt32> cacheids = { from_t->type_id, target_t->type_id };

		auto & cacheid = Reinterpret<UInt64>(cacheids);

		if (auto ptyperef = cache.SearchValue(cacheid))
		{
			castcount += ptyperef->b;

			return ptyperef->a;
		}

		InstructionsRestorePoint restore(scope);

		UInt castcountx = 0;

		bool cancast = true;

		try
		{
			ExpectCast(src, scope, variable, target_t, castcountx);
		}
		catch (const SyntaxError &)
		{
			cancast = false;
		}

		restore.Rollback();

		castcount += castcountx;

		return cache.Insert(cacheid, { cancast, castcountx }).a;
	}
}

Variable CompilerImpl::ExpressionHandler::ExpectCast(Source & src, Scope & scope, const Variable & variable, TypeRef target_t, UInt & castcount)
{
	auto from_t = variable.type;

	REFLEX_ASSERT(target_t);

	REFLEX_ASSERT(target_t != from_t);



	//try cast
	
	castcount += 2;	//Object downcast is less weighted than a conversion cast function



	//decay to void

	auto & bindings = scope.bindings;

	if (target_t == bindings.void_t)
	{
		AddDiscardVariable(scope.instructions, src, from_t);

		return Private::MakeTemporary(target_t);
	}


	
	//inheritance

	auto & state = scope.state;

	if (state.InheritsFrom(from_t, target_t))
	{
		castcount--;

		//NOTE if supporting value inheritance, would need to pop stack of derived part
		//this could be a unified solution for cast to void, which should really be cast to 'NothingType', to distinquish from actual void return (which feels like bug)

		return { target_t, variable.address, variable.location, variable.is_const };
	}



	//cast operator and bool special handling 

	Variable resolved = variable;

	ResolveIfObjectTemporary(src, scope, resolved);	//TEST


	
	//case operator

	IS_RETAINED(resolved);

	CallHandler::Input input(src, scope);

	input.AddTArg(target_t);

	input.AddCaller(resolved);

	input.cancastfn = [](Source & src, Scope & scope, const Variable & from, TypeRef target_t, UInt & castcount)
	{
		return from.type == target_t;
	};

	if (auto result_t = CallHandler::Resolve({}, kopCast, input))
	{
		return Private::MakeTemporary(result_t);
	}

		

	//bool

	if (target_t == bindings.bool_t)
	{
		if (from_t->IsObject())
		{
			IS_RETAINED(resolved);

			CallHandler::Input input(src, scope);

			input.AddCaller(resolved);

			input.Add(resolved);//could be resolved

			input.cancastfn = [](Source & src, Scope & scope, const Variable & from, TypeRef target_t, UInt & castcount)
			{
				if (from.type->IsObject() && target_t == scope.bindings.bool_t)
				{
					return false;
				}
				else
				{
					return CanCast(src, scope, from, target_t, castcount);
				}
			};

			AddPushNull(scope.instructions, src, from_t);

			if (CallHandler::Resolve({}, kopInequal, input) == bindings.bool_t)
			{
				return Private::MakeTemporary(target_t);
			}
		}
		else 
		{
			AddPushConst(scope.instructions, src, from_t->params);

			AddComparisonIntrinsic(scope.instructions, src, variable.type, OPCODE(intrinsicValueInequal));

			return Private::MakeTemporary(target_t);
		}
	}

	scope.ThrowCustomError(src, kErrorCanNotCast, { from_t->name, target_t->name });

	return variable;
}

Variable CompilerImpl::ExpressionHandler::ResolveTemporary(Source & src, Scope & scope, const Variable & variable, bool byref)
{
	REFLEX_ASSERT(variable.location == Location::kTemporary || byref);

	auto var = scope.RegisterVariable(src, scope.state.AcquireAnonSymbol(), variable.type);

	auto & instructions = scope.instructions;

	AddAssignVariable(instructions, src, var);

	AddPushVariable(instructions, src, var, byref);

	return var;
};

REFLEX_INLINE Variable CompilerImpl::ExpressionHandler::Private::DispatchBracketConstructor(Source & src, Itr & itr, Scope & scope, TypeRef lhs, UInt32 type)
{
	INLINE(Variable, DispatchBraces)(Source & src, Scope & scope, TypeRef lhs, Itr inner)
	{
		//INITALISER LIST

		if (CallHandler::IsAssociativeArguments(inner))
		{
			CallHandler::Input associative(src, scope);// BuildAssociativeInput(src, scope, inner)

			CallHandler::BuildAssociativeArguments(associative, inner);

			auto type = Assume(CallHandler::ResolveOperator(ToView(kGlobal), kopCreatePropertySet, associative), SyntaxError(src));

			return MakeTemporary(type);
		}
		else if (lhs)
		{
			if (lhs->IsObject())
			{
				return MakeTemporary(AssumeConstructor(src, scope, lhs, inner));
			}
			else
			{
				CallHandler::Input input(src, scope);

				CallHandler::BuildRegularArguments(input, inner);

				Assume(input.GetSlots().GetSize() <= lhs->members.GetSize(), TypeError(src));

				auto & instructions = scope.instructions;

				UInt size = 0;

				auto members = lhs->members.GetData();

				for (auto & i : input.GetSlots())
				{
					auto & member = *members++;

					auto member_t = member.b.type;

					if (auto type = i.variable.type)
					{
						Assume(type == member_t, TypeError(src));

						size += type->size;

						instructions.Append(i.instructions);
					}
					else
					{
						ExpressionHandler::RenderScope render(scope, i.instructions);

						auto result_t = render.Parse2(src, Copy(i.itr), member_t).type;

						Assume(result_t == member_t, TypeError(src));

						size += result_t->size;

						instructions.Append(i.instructions);
					}
				}

				auto remainder = Splice(lhs->params, size).b;

				AddPushConst(instructions, src, remainder);

				return MakeTemporary(lhs);
			}
		}

		UNREACHABLE_X(TypeError, kErrorCanNotDeduceType);
	}
	END

	INLINE(Variable, DispatchSquare)(Source & src, Scope & scope, TypeRef lhs, Itr & itr, Itr inner)
	{
		//need to branch array vs anon function
		//array: assume []; or [], or []
		//anonfn: assume []() or []word

		auto & state = scope.state;

		auto next = Inc(src, itr).GetNext();

		if (!next || next->type == Token::kTypeSymbol)
		{
			auto & info = kCreateArrayInfo[CallHandler::IsAssociativeArguments(inner)];

			CallHandler::Input input(src, scope);

			info.c(input, inner, kMaxUInt32);

			if (auto template_type = state.GetInstantiatedTemplateType(lhs))
			{
				//PROXY
				//real solution lookup(LHS::opCreateFromArray, input)

				for (auto & i : template_type->targs)
				{
					input.AddTArg(i);
				}
			}
			else
			{
				auto & args = input.GetSlots();

				auto n = Min(info.a, args.GetSize());

				REFLEX_LOOP_PTR(args.GetData(), parg, n)
				{
					if (auto type = parg->variable.type)
					{
						input.AddTArg(type);
					}
					else
					{
						input.ClearTArgs();

						break;
					}
				}
			}

			Assume(input.GetTArgs().GetSize() == info.a, TypeError(src, kErrorTypeMismatch));

			if (auto type = CallHandler::ResolveOperator(ToView(kGlobal), info.b, input))
			{
				return MakeTemporary(type);
			}
			else
			{
				TypeError::Throw(src, kErrorOperatorNotFound);
			}
		}
		else
		{
			//TODO enforce capturing of temp/local vars only

			Array <VarRef> captures(scope.allocator);

			while (inner)
			{
				auto var = ExpressionHandler::ParseX(src, inner, scope, ExpressionHandler::kContextConditional, 0);

				Assume(var.location == Location::kLocal || var.location == Location::kGlobal, SyntaxError(src, kErrorExpectedLocalVariable));

				auto pscope = &scope;

				do
				{
					for (auto & i : pscope->variables)
					{
						auto symbol = i.a;

						if (i.b.address == var.address)
						{
							REFLEX_ASSERT(i.b.type == var.type);

							captures.Push({ symbol.b, var.type });	//AddPushVariable(scope.instructions, src, var, false)

							goto Next;
						}
					}
				}
				while ((pscope = pscope->IsSub() ? pscope->GetPrev() : 0));

				SyntaxError::Throw(src, kErrorExpectedVariable);

				REFLEX_MARKER(Next);

				Assume(True(var.type), SyntaxError(src, kErrorExpectedVariable));

				if (inner)
				{
					Expect<Token::kTypeSymbol>(src, inner, kSymbolComma);
				}
			}

			auto rtn_t = scope.ParseType(src, itr);

			auto args = Assume(BracketScope(src, itr, kBracketRound), SyntaxError(src, kErrorExpectedAnonFunction)).inner;

			auto & arguments = FunctionHandler::AssumeArguments(src, args, scope);

			Array <TypeRef> types(scope.allocator);

			types.Allocate(captures.GetSize());

			for (auto & i : captures)
			{
				arguments.Push(Argument(i.b, false, i.a));

				types.Push(i.b);
			}

			auto name = AcquireStaticString(state, Join("anon", Data::BytesToHex(Data::Pack(state.AcquireAnonSymbol()))));

			auto & fn = FunctionHandler::Retrieve(src, scope, scope.GetNamespace(), name, {}, rtn_t, arguments);

			Assume(FunctionHandler::ParseBody(src, itr, scope, fn), SyntaxError(src, kErrorExpectedAnonFunction));

			return MakeTemporary(ApplyBind(src, scope, itr, fn, types));
		}

		UNREACHABLE;
	}
	END

	Itr inner = itr->GetFirst();

	switch (type)
	{
	case kBracketRound:
		Assume(Copy(inner), SyntaxError(src));
		Inc(src, itr);
		return Parse(src, inner, scope, kContextTemporary, lhs);

	case kBracketBrace:
		Inc(src, itr);
		return DispatchBraces::Call(src, scope, lhs, inner);

	case kBracketSquare:
		return DispatchSquare::Call(src, scope, lhs, itr, inner);
	}

	UNREACHABLE;
}

template <class T> void Invert(const T * t)
{
	RemoveConst(*t) = -*t;
}

REFLEX_INLINE Variable CompilerImpl::ExpressionHandler::Private::DispatchKeyword(Source & src, Itr & itr, Scope & scope, Context context, TypeRef lhs, UInt32 keyword)
{
	INLINE(TypeRef,DispatchNew)(Source & src, Itr & itr, Scope & scope, TypeRef lhs)
	{
		constexpr auto ExpectConstructor = [](Source & src, Scope & scope, Itr & itr, TypeRef type) -> TypeRef
		{
			BracketScope bracketscope(src, itr, kBracketBrace);

			return AssumeConstructor(src, scope, type, bracketscope.inner);
		};

		if (auto type = scope.ParseType(src, itr))
		{
			return ExpectConstructor(src, scope, itr, type);
		}
		else if (itr && itr->type == kTypeWord)
		{
			UNREACHABLE_X(SyntaxError, kErrorExpectedType);
		}
		else if (lhs)
		{
			return ExpectConstructor(src, scope, itr, lhs);
		}
		else
		{
			UNREACHABLE_X(TypeError, kErrorCanNotDeduceType);
		}
	}
	END

	INLINE(TypeRef,Dispatch_null)(Source & src, Itr & itr, Scope & scope, TypeRef lhs)
	{
		auto PushNull = [](Source & src, Scope & scope, TypeRef type)
		{
			Assume(IsExplicitNullable(scope.bindings, type), SyntaxError(src, kErrorNonNullableType));

			AddPushNull(scope.instructions, src, type);

			return type;
		};

		if (auto type = scope.ParseType(src, itr))
		{
			return PushNull(src, scope, type);
		}
		else if (itr && itr->type == kTypeWord)
		{
			UNREACHABLE_X(SyntaxError, kErrorExpectedType);
		}
		else if (lhs)
		{
			return PushNull(src, scope, lhs);
		}
		else
		{
			UNREACHABLE_X(TypeError, kErrorCanNotDeduceType);
		}
	}
	END

	INLINE(void, Dispatch_const)(Source & src, Itr & itr, Scope & scope)
	{
		CString::View name;

		auto & state = scope.state;

		auto type = Assume(scope.ParseTypeOrAuto(src, itr), SyntaxError(src, kErrorExpectedType));

		name = Assume(Traverse<Token::kTypeWord>(src, itr), SyntaxError(src, kErrorExpectedName))->value;

		Assume(!state.GetConstValue({ scope.GetNamespace(), name }), SyntaxError(src, kErrorDuplicateSymbol));

		Assume(Traverse<Token::kTypeSymbol>(src, itr, kSymbolEqual), SyntaxError(src, kErrorExpectedAssignmentOperator));


		auto marker = scope.instructions.GetSize();

		auto result = Parse(src, itr, scope, kContextStatement, type);

		auto temp = Splice(scope.instructions, marker).b;


		auto result_t = Assume(Copy(result.type), SyntaxError(src, kErrorExpectedConstant));

		Assume(state.IsAuto(type) || type == result_t, SyntaxError(src, kErrorTypeMismatch));

		if (result.location == Location::kConst)
		{
			auto i = temp.GetLast();

			scope.state.RegisterConstant(result_t, scope.GetNamespace(), name, { &Reinterpret<UInt8>(i.param64), Reinterpret<Pair<UInt16>>(i.param32).b });// .documentable = false;
		}
		else if (result.location == Location::kTemporary)
		{
			Assume(!result_t->IsObject(), SyntaxError(src, kErrorExpectedConstant));

			if (temp.size == 1)
			{
				auto i = temp.GetLast();

				scope.state.RegisterConstant(result_t, scope.GetNamespace(), name, { &Reinterpret<UInt8>(i.param64), Reinterpret<Pair<UInt16>>(i.param32).b });// .documentable = false;
			}
			else
			{
				Data::Archive accumulate;

				for (auto & i : temp)
				{
					switch (i.opcode)
					{
					case OPCODE(PushConst):
						accumulate.Append({ &Reinterpret<UInt8>(i.param64), Reinterpret<Pair<UInt16>>(i.param32).b });
						break;

					case OPCODE(intrinsicInvertFloat32):
						Invert(Reinterpret<Float32>(Right(accumulate, 4).data));
						break;

					case OPCODE(intrinsicInvertInt32):
						Invert(Reinterpret<Int32>(Right(accumulate, 4).data));
						break;

					default:
						SyntaxError::Throw(src, kErrorExpectedConstant);
						break;
					}
				}

				Assume(accumulate.GetSize() <= 16, SyntaxError(src, kErrorInvalidSize));

				scope.state.RegisterConstant(result_t, scope.GetNamespace(), name, accumulate);// .documentable = false;
			}
		}
		else
		{
			SyntaxError::Throw(src, kErrorExpectedConstant);
		}

		scope.instructions.SetSize(marker);
	}
	END

	INLINE(TypeRef,Dispatch_typeid)(Source & src, Itr & itr, Scope & scope, TypeRef lhs)
	{
		auto type = ExpectType(src, itr, scope);

		AddPushConst32(scope.instructions, src, type->type_id);

		return scope.bindings.key32_t;
	}
	END

	auto word = Inc(src, itr).value;

	switch (keyword)
	{
	case knew:
		return MakeTemporary(DispatchNew::Call(src, itr, scope, lhs));

	case kconst:
		Dispatch_const::Call(src, itr, scope);
		return MakeTemporary(scope.bindings.void_t);

	case knull:
		return MakeTemporary(Dispatch_null::Call(src, itr, scope, lhs));

	case kbind:
		return MakeTemporary(Dispatch_bind(src, itr, scope, lhs));

	case ktypeid:
		return MakeTemporary(Dispatch_typeid::Call(src, itr, scope, lhs));

	default:
		scope.state.ThrowCustomError(src, kErrorTemplateInvalidKeyword, { word });
		break;
	}

	UNREACHABLE;
}

REFLEX_INLINE Variable CompilerImpl::ExpressionHandler::Private::DispatchWord(Source & src, Itr & itr, Scope & scope, Context context, TypeRef lhs, UInt32 token)
{
	if (auto type = scope.ParseType(src, itr))
	{
		return DispatchWord_TypedVarDecl(src, itr, scope, context, type);
	}
	else if (token == kauto)
	{
		Assume(context != kContextTemporary, SyntaxError(src));

		Inc(src, itr);

		auto & name = *Assume(Traverse<Token::kTypeWord>(src, itr), SyntaxError(src, kErrorExpectedName));

		if constexpr (REFLEX_DEBUG) scope.state.RegisterStaticString(name.hash, name.value);

		Assume(Traverse<Token::kTypeSymbol>(src, itr, kSymbolEqual), SyntaxError(src, kErrorExpectedAssignmentOperator));

		auto type = Assume(ExpressionHandler::Parse(src, itr, scope, Context(kContextAssignment), 0), SyntaxError(src));

		auto var = scope.RegisterVariable(src, name.value, type);

		auto & instructions = scope.instructions;

		AddAssignVariable(instructions, src, var);

		AddPushVariable(instructions, src, var, false);

		return var;
	}
	else
	{
		typedef Sequence<UInt64,ExternalObject>::ConstRange ExternalObjects;

		auto pair = ParseNamespacedSymbol(src, itr);	//ns + symbol

		auto pglobal = scope.state.root;

		Scope * null = nullptr;

		Scope::SymbolAccessor::Param param = scope.IsGlobal() ? MakeTuple(pglobal, null) : MakeTuple(&scope.GetRoot(), pglobal);

		auto symbolref = GetSymbolsOfType<Scope::SymbolAccessor>(scope, pair.a, pair.b, scope.GetUsings(), param);

		switch (symbolref.a)
		{
		case Scope::kSymbolTypeVariable:
			AddPushVariable(scope.instructions, src, Reinterpret<Variable>(symbolref.b), false);
			return Reinterpret<Variable>(symbolref.b);
			break;

		case Scope::kSymbolTypeConstant:
		{
			auto & bindings = scope.bindings;

			auto & constant = *Reinterpret<State::Constant*>(symbolref.b);

			auto type = constant.type;

			if (constant.type == bindings.uint8_t && lhs == bindings.int32_t) type = lhs;

			AddPushConst(scope.instructions, src, { Reinterpret<UInt8>(&constant.value), type->size });

			return MakeConst(constant.type);
		}
		break;

		default:
			if (itr && (itr->hash == kBracketRound || itr->hash == kSymbolAt))
			{
				auto type = Private::CallFunction(src, scope, itr, pair);

				return MakeTemporary(type);
			}
			else
			{
				scope.state.ThrowCustomError(src, kErrorTemplateUnknownSymbol, { pair.b });
			}
			break;
		}
	}

	UNREACHABLE;
}

REFLEX_INLINE Variable CompilerImpl::ExpressionHandler::Private::DispatchWord_TypedVarDecl(Source & src, Itr & itr, Scope & scope, Context context, TypeRef variable_t)
{
	LOCAL(Variable, AddVar)(Source & src, Itr & itr, Scope & scope, Context context, TypeRef type)
	{
		Assume(context != kContextTemporary, SyntaxError(src));

		auto & name = *Assume(Traverse<Token::kTypeWord>(src, itr), SyntaxError(src, kErrorExpectedName));

		auto var = scope.RegisterVariable(src, name.value, type);

		if (var.type->IsObject() && !Peek(src, itr, kSymbolEqual))
		{
			AssumeConstructor(src, scope, var.type, 0);

			AddAssignVariable(scope.instructions, src, var);
		}

		if (Traverse<Token::kTypeSymbol>(src, itr, kSymbolComma))
		{
			return Call(src, itr, scope, context, type);
		}
		else
		{
			AddPushVariable(scope.instructions, src, var, false); //for discard?

			return var;
		}
	}
	END

	switch (context)
	{
	case kContextStatement:
	case kContextConditional:
		return AddVar::Call(src, itr, scope, context, variable_t);

	default:
		SyntaxError::Throw(src);
		break;
	}

	UNREACHABLE;
}

Variable CompilerImpl::ExpressionHandler::Private::Parse(Source & src, Itr & itr, Scope & scope, Context context, TypeRef lhs)
{
	INLINE(Variable, AddVar)(Source & src, Itr & itr, Scope & scope, Context context, TypeRef type)
	{
		Assume(context != kContextTemporary, SyntaxError(src));

		auto & name = *Assume(Traverse<Token::kTypeWord>(src, itr), SyntaxError(src, kErrorExpectedName));

		auto var = scope.RegisterVariable(src, name.value, type);

		if (var.type->IsObject() && !Peek(src, itr, kSymbolEqual))
		{
			AssumeConstructor(src, scope, var.type, 0);

			AddAssignVariable(scope.instructions, src, var);
		}

		if (Traverse<Token::kTypeSymbol>(src, itr, kSymbolComma))
		{
			return Call(src, itr, scope, context, type);
		}
		else
		{
			AddPushVariable(scope.instructions, src, var, false); //for discard?

			return var;
		}
	}
	END

	INLINE(Variable, DispatchSingleQuotedString)(Source & src, Itr & itr, Scope & scope)
	{
		AddPushConst32(scope.instructions, src, Inc(src, itr).hash.value);

		return MakeConst(scope.bindings.key32_t);
	}
	END

	INLINE(Variable, DispatchDoubleQuotedString)(Source & src, Itr & itr, Scope & scope)
	{
		auto & state = scope.state;

		auto & bindings = scope.bindings;

		auto & value = Inc(src, itr).value;

		auto & conststring = state.AquireStringLiteral(value, ToWString(value));

		AddPushConst(scope.instructions, src, Data::Pack(&conststring));

		return MakeConst(bindings.string_t);
	}
	END

	INLINE(Variable, DispatchSymbol)(Source & src, Itr & itr, Scope & scope, Context context, UInt32 token)
	{
		switch (token)
		{
		case kSymbolAt:
			{
				Inc(src, itr);

				auto type = ExpectType(src, itr, scope);

				return ParseX(src, itr, scope, context, type);
			}
			break;

		case kSymbolNamespaceDelimiter:
			{
				Inc(src, itr);

				SubScope subscope(scope);

				subscope.RegisterUseGlobal();

				return ParseX(src, itr, subscope, context, 0);
			}
			break;

		case kSymbolHash:
			{
				Inc(src, itr);

				Assume(itr && itr->type == Token::kTypeWord, SyntaxError(src, kErrorExpectedName));

				AddPushConst32(scope.instructions, src, itr->hash.value);

				Inc(src, itr);

				return MakeConst(scope.bindings.key32_t);
			}

		case kSymbolSemiColon:
			return MakeTemporary(scope.bindings.void_t);

		default:
			if (auto type = TraversePreOperator(src, itr, scope, token))	//	//just function call with 'fancy syntax'
			{
				return MakeTemporary(type);
			}
			else
			{
				auto token = Inc(src, itr).value;

				scope.state.ThrowCustomError(src, "unexpected symbol [0]", { token });
			}
		}

		UNREACHABLE;
	}
	END

	Variable result = MakeTemporary(scope.bindings.void_t);

	if (!itr) return result;

	auto tokentype = itr->type;

	auto token = itr->hash.value;

	switch (tokentype)
	{
		DISPATCH(Token::kTypeBracket, DispatchBracketConstructor(src, itr, scope, lhs, token));

		DISPATCH(Token::kTypeSingleQuotedString, DispatchSingleQuotedString::Call(src, itr, scope));

		DISPATCH(Token::kTypeDoubleQuotedString, DispatchDoubleQuotedString::Call(src, itr, scope));

		DISPATCH(Token::kTypeSymbol, DispatchSymbol::Call(src, itr, scope, context, token));

		DISPATCH(Token::kTypeInt, DispatchNumberLiteral<false>(src, itr, scope));

		DISPATCH(Token::kTypeFloat, DispatchNumberLiteral<true>(src, itr, scope));

		DISPATCH(Token::kTypeKeyword, DispatchKeyword(src, itr, scope, context, lhs, token));

		DISPATCH(Token::Token::kTypeWord, DispatchWord(src, itr, scope, context, lhs, token));

		default: UNREACHABLE;
	}


	//-------------------------------
	//CHAIN RECURSION
	//now we have a variable/object on the stack
	//chain can continue in following ways

	//.operator then function or member
	//[syntax] which is get var.Get("pun").Get("asm")  = var["pun"]["pun"]

	//**** TESTING ****//
	//
	// byref issue
	// module["updates"] =  module[@Int32:"updates"] + 1;
	// so not to parse chain. but post op needs it.  but what about if already 'consumed' by above places ?
	//

	if (auto var = ParseChain(src, scope, itr, lhs, result))
	{
		result = var;
	}

	if (itr && itr->type == Token::kTypeSymbol)
	{
		auto symbol = itr->hash.value;

		if (auto type = TraversePostOperator(src, itr, scope, context, symbol, result))
		{
			result = MakeTemporary(type);
		}
		else if (symbol == kSymbolEqual)
		{
			Assume(context != kContextTemporary, SyntaxError(src));

			Assume(result.location < Location::kConst, SyntaxError(src, kErrorNonAssignableValue));

			Inc(src, itr);

			auto & instructions = scope.instructions;	//remove Pushed parameter, because Assign opcode does this

			instructions.Pop();

			Assume(ExpressionHandler::Parse(src, itr, scope, Context(kContextAssignment), result.type) == result.type, TypeError(src));

			AddAssignVariable(instructions, src, result);

			if (context == kContextConditional && result.location != Location::kMember)
			{
				AddPushVariable(instructions, src, result, false);
			}
			else
			{
				result = MakeTemporary(scope.bindings.void_t);
			}
		}
	}

	if (context < kNumContext)	//IF *NOT* ASSIGNMENT
	{
		ResolveIfObjectTemporary(src, scope, result);
	}

	return result;
}

Variable CompilerImpl::ExpressionHandler::Private::ParseChain(Source & src, Scope & scope, Itr & itr, TypeRef lhs, Variable caller)
{
	INLINE(Variable,TraverseMember)(Source & src, Scope & scope, Itr & itr, Variable & prev)
	{
		if (auto word = Peek(src, itr, Token::kTypeWord))
		{
			auto & chain = scope.state.RetrieveInheritanceInfo(prev.type).a;

			REFLEX_FOREACH(type, chain)
			{
				REFLEX_FOREACH(item, type->members)
				{
					if (item.a == word->hash)
					{
						Inc(src, itr);

						return item.b;
					}
				}
			}
		}

		return {};
	}
	END

	INLINE(TypeRef, TraversePropertySelector)(Source & src, Scope & scope, Itr & itr, TypeRef & lhs, Variable caller)
	{
		if (Traverse<Token::kTypeSymbol>(src, itr, kSymbolHash))
		{
			auto id = Assume(Traverse<Token::kTypeWord>(src, itr), SyntaxError(src, kErrorExpectedName))->hash;

			ResolveIfObjectTemporary(src, scope, caller);

			IS_RETAINED(caller);

			CallHandler::Input input(src, scope);

			AddPushConst32(scope.instructions, src, id.value);

			input.AddCaller(caller);

			input.Add({ scope.bindings.key32_t });

			if (Traverse<Token::kTypeSymbol>(src, itr, kSymbolEqual))
			{
				input.Add(itr, ToView(CallHandler::Input::kDelimiterSemiColon), false);

				return Assume(CallOperatorMethod(src, scope, kopSet, input), SyntaxError(src, kErrorOperatorNotFound));
			}
			else if (lhs)
			{
				input.AddTArg(lhs);

				if (auto result_t = CallOperatorMethod(src, scope, kopGet, input))
				{
					lhs = 0;	//consumed lhs !

					return result_t;
				}

				input.ClearTArgs();
			}

			return Assume(CallOperatorMethod(src, scope, kopGet, input), TypeError(src, kErrorOperatorNotFound));
		}

		return 0;
	}
	END

	INLINE(TypeRef,TraverseElementAccessor)(Source & src, Scope & scope, Itr & itr, TypeRef & lhs, Variable caller)
	{
		if (auto bracketscope = BracketScope(src, itr, kBracketSquare))	//Get and Set shortcut (Set = "obj[idx] = y")
		{
			ResolveIfObjectTemporary(src, scope, caller);	//TEST

			IS_RETAINED(caller);

			CallHandler::Input input(src, scope);

			CallHandler::ParseTArgs(src, bracketscope.inner, input);

			input.AddCaller(caller);

			input.Add(bracketscope.inner, {}, false);	//only 1 arg, so no delimiter

			if (Traverse<Token::kTypeSymbol>(src, itr, kSymbolEqual))
			{
				input.Add(itr, ToView(CallHandler::Input::kDelimiterSemiColon), false);

				return Assume(CallOperatorMethod(src, scope, kopSet, input), SyntaxError(src, kErrorOperatorNotFound));
			}
			else if (!lhs || input.GetTArgs())
			{
				return Assume(CallOperatorMethod(src, scope, kopGet, input), TypeError(src, kErrorOperatorNotFound));
			}
			else
			{
				input.AddTArg(lhs);

				if (auto result_t = CallOperatorMethod(src, scope, kopGet, input))
				{
					lhs = 0;

					return result_t;
				}
				else
				{
					input.ClearTArgs();

					return Assume(CallOperatorMethod(src, scope, kopGet, input), TypeError(src, kErrorOperatorNotFound));
				}
			}
		}

		return 0;
	}
	END

	Variable next;

	auto is_object = caller.type->IsObject();

	if (Traverse<Token::kTypeSymbol>(src, itr, K32(".")))
	{
		if (auto var = TraverseMember::Call(src, scope, itr, caller))
		//selecting .member
		{
			auto & instructions = scope.instructions;

			if (is_object)
			{
				ResolveIfObjectTemporary(src, scope, caller);

				REFLEX_ASSERT(var.location == Location::kMember);

				AddPushVariable(instructions, src, var, false);
			}
			else
			{
				auto Apply = [](Instruction & instruction, Variable & prev, Variable & var)
				{
					auto & info = Reinterpret< Pair<UInt16> >(instruction.param32);

					info.a += var.address;

					info.b = var.type->size;

					var.address += prev.address;	//for assign
				};

				//this *must* be a subvalue, because structs cant have objects
				//so top of the stacks must be a value, because return by ref not permitted

				var.location = caller.location;

				auto & instruction = instructions.GetLast();

				switch (instruction.opcode)
				{
				case OPCODE(PushGlobal):
				case OPCODE(PushLocal):
				case OPCODE(PushMember):
				case OPCODE(PushGlobalByAdr):
				case OPCODE(PushLocalByAdr):
				case OPCODE(PushMemberByAdr):
					REFLEX_ASSERT(caller.location != Location::kTemporary);
					Apply(instruction, caller, var);
					break;

				case OPCODE(SwizzleTemporaryValue):
					REFLEX_ASSERT(caller.location == Location::kTemporary || caller.location == Location::kConst);
					Apply(instruction, caller, var);
					break;

				default:
					Apply(instructions.Push({ src.line, src.file, OPCODE(SwizzleTemporaryValue), MakeParam32(0, caller.type->size), MakeParam64(caller.type) }), caller, var);

					//TODO move this check to logical check by TypeX info
					Assume(!Traverse<Token::kTypeSymbol>(src, itr, kSymbolEqual), SyntaxError(src, kErrorCanNotAssignTemporary));
					break;

				//case OPCODE(intrinsicValueArrayGet):
				//case OPCODE(intrinsicValueArray32Get):
				//case OPCODE(CallFn):
				//case OPCODE(CallFnObject):
				//case OPCODE(ReturnExternalMember):
				//	Reinterpret<Pair<UInt8>>(instruction.flags).a += var.address;
				//	Reinterpret<Pair<UInt8>>(instruction.flags).b = var.type->size;	//instruction.flags = prev.type->size - (var.address +
				//	//TODO return VAR with info on location (external temporaty), then assign will not work
				//	Assume(!Traverse<Token::kTypeSymbol>(src, itr, kSymbolEqual), SyntaxError(src, kErrorCanNotAssignTemporary));
				//	//Assume(!byref, SyntaxError(src, kErrorCanNotPassTemporaryByRef));
				//	break;
				}
			}

			GOTO(End, next, var);
		}
		else
		//selecting .method()
		{
			ResolveIfObjectTemporary(src, scope, caller);

			Assume(itr && itr->type == Token::kTypeWord, SyntaxError(src));

			CString::View null;

			CallHandler::Input input(src, scope);

			input.AddCaller(caller);

			if (auto type = CallHandler::DispatchCall(src, scope, itr, { null, Inc(src, itr).value }, input))
			{
				GOTO(End, next, MakeTemporary(type));
			}
		}

		SyntaxError::Throw(src, kErrorExpectedMemberOrMethod);
	}
	else if (auto type = TraverseElementAccessor::Call(src, scope, itr, lhs, caller))
	{
		GOTO(End, next, MakeTemporary(type));
	}
	else if (auto type = TraversePropertySelector::Call(src, scope, itr, lhs, caller))
	{
		GOTO(End, next, MakeTemporary(type));
	}
	else if (auto inner = BracketScope(src, itr, kBracketRound))
	{
		ResolveIfObjectTemporary(src, scope, caller);

		CallHandler::Input input(src, scope);

		input.AddCaller(caller);

		CallHandler::BuildRegularArguments(input, inner.inner);

		auto type = Assume(CallOperatorMethod(src, scope, kopInvoke, input), SyntaxError(src, kErrorOperatorNotFound));

		GOTO(End, next, MakeTemporary(type));
	}
	else
	{
		return {};
	}

	REFLEX_MARKER(End);

	if (auto var = ParseChain(src, scope, itr, lhs, next))
	{
		return var;
	}
	else
	{
		return next;
	}
}

TypeRef CompilerImpl::ExpressionHandler::Private::Dispatch_bind(Source & src, Itr & itr, Scope & scope, TypeRef lhs)
{
	//TODO this is quick hack, only finds 1 candidate.  after INPUT cleanup, unify with callhandler

	struct ScriptFunctionAccessor
	{
		typedef Key32 SymbolType;

		typedef ArrayView <Argument> Param;

		typedef Array <const Function *> Output;

		static bool Get(Scope & scope, Symbol symbol, const Param & targs, Output & output)
		{
			for (auto & i : scope.state.m_target->GetScriptFunctions(symbol))
			{
				output.Push(&i.value);
			}

			return true;
		}
	};

	struct FunctionAccessor
	{
		typedef Key32 SymbolType;

		typedef ArrayView <Argument> Param;

		typedef Array <const Function *> Output;

		static bool Get(Scope & scope, Symbol symbol, const Param & args, Output & output)
		{
			for (auto & i : scope.bindings.GetExternalFunctions(symbol))
			{
				output.Push(&i.value);
			}

			return true;
		}
	};

	RestorePoint restore(scope, itr);

	auto a = &Scope::TraverseSymbolsOfType<ScriptFunctionAccessor>;

	auto b = &Scope::TraverseSymbolsOfType<FunctionAccessor>;

	decltype (&Scope::TraverseSymbolsOfType<ScriptFunctionAccessor>) accessors[2] = { a,  b };

	REFLEX_LOOP(idx, 2)
	{
		if (auto candidates = (scope.*accessors[idx])(src, itr, {}))
		{
			auto targs = FunctionHandler::ParseTArgs(src, itr, scope);

			for (auto & i : candidates)
			{
				if (targs == i->targs)
				{
					Array <TypeRef> captures(scope.allocator);

					if (auto bracketscope = BracketScope(src, itr, kBracketSquare))
					{
						CallHandler::Input input(src, scope);

						auto & args = i->args;

						CallHandler::BuildRegularArguments(input, bracketscope.inner, args.GetSize());

						if (bracketscope.inner) goto Next;

						auto parg = args.GetData() + (args.GetSize() - input.GetSlots().GetSize());

						for (auto & i : input.GetSlots())
						{
							captures.Push(Assume(ExpressionHandler::Parse(src, i.itr, scope, kContextTemporary, parg->type), SyntaxError(src, kErrorExpectedType)));

							parg++;
						}
					}

					return ApplyBind(src, scope, itr, *i, captures);
				}
			}

			REFLEX_MARKER(Next);

			restore.Rollback();
		}
	}

	scope.state.ThrowCustomError(src, kErrorTemplateUnknownSymbol, { restore.store->value });

	return {};
}

template <bool _32BIT> TypeRef CompilerImpl::ExpressionHandler::Private::PushNumberLiteral(Scope & scope, Source & src, TypeRef type, const void * pvalue)
{
	auto & instructions = scope.instructions;

	if constexpr (_32BIT)
	{
		AddPushConst32(instructions, src, *Reinterpret<UInt32>(pvalue));
	}
	else
	{
		UInt size = type->size;

		UInt8 copy[8] = { 0 };

		auto dst = copy;

		REFLEX_LOOP_PTR(Reinterpret<UInt8>(pvalue), src, size) *dst++ = *src;

		AddPushConst(instructions, src, { copy, size });
	}

	return type;
}

template <bool FLOAT> Variable CompilerImpl::ExpressionHandler::Private::DispatchNumberLiteral(Source & src, Itr & itr, Scope & scope)
{
	auto & bindings = scope.bindings;

	auto & value = Inc(src, itr).value;

	if (auto suffix = Traverse<Token::kTypeWord>(src, itr))
	{
		if constexpr (FLOAT)
		{
			if (suffix->hash == K32("f"))
			{
				Float32 f = ToFloat32(value);

				return MakeConst(PushNumberLiteral<true>(scope, src, bindings.float32_t, &f));
			}
		}
		else if (suffix->hash == K32("ub"))
		{
			UInt8 ub = UInt8(ToUInt32(value));

			return MakeConst(PushNumberLiteral<false>(scope, src, bindings.uint8_t, &ub));
		}
	}
	else
	{
		if constexpr (!FLOAT)
		{
			Int32 i = ToInt32(value);

			return MakeConst(PushNumberLiteral<true>(scope, src, bindings.int32_t, &i));
		}
	}
		
	SyntaxError::Throw(src, kErrorInvalidNumberSuffix);

	return MakeConst(bindings.void_t);
}

TypeRef CompilerImpl::ExpressionHandler::Private::TraversePreOperator(Source & src, Itr & itr, Scope & scope, UInt32 token)
{
	INLINE(TypeRef, Invoke)(Source & src, Scope & scope, Itr & itr, Key32 symbol)
	{
		CallHandler::Input input(src, scope);

		input.Add(itr, { CallHandler::Input::kDelimiterSemiColon, CallHandler::Input::kDelimiterComma }, false);

		return Assume(CallHandler::Resolve({}, symbol, input), SyntaxError(src, kErrorOperatorNotFound));
	}
	END

	auto & bindings = scope.bindings;

	switch (token)
	{
	case K32("!"):
		{
			Inc(src, itr);

			auto result = ExpectConditionalExpression(src, itr, scope, ExpressionHandler::kContextTemporary);

			CallHandler::Input input(src, scope);

			input.Add(result);

			Assume(CallHandler::Resolve({}, kopLogicalNot, input) == bindings.bool_t, SyntaxError(src, kErrorOperatorNotFound));
		}
		return bindings.bool_t;

	case K32("++"):
		Inc(src, itr);
		return Invoke::Call(src, scope, itr, kopPreInc);

	case K32("--"):
		Inc(src, itr);
		return Invoke::Call(src, scope, itr, kopPreDec);

	case K32("-"):
		Inc(src, itr);
		return Invoke::Call(src, scope, itr, kopInvert);

	default:
		return 0;
	}
}

TypeRef CompilerImpl::ExpressionHandler::Private::TraversePostOperator(Source & src, Itr & itr, Scope & scope, Context context, UInt32 symbol, Variable var)
{
	Key32 intrinsic;

	switch (symbol)
	{
	case K32("++"):
	case K32("--"):
		{
			auto & last = scope.instructions.GetLast().opcode;

			switch (last)
			{
			case OPCODE(PushGlobal):
			case OPCODE(PushLocal):
			case OPCODE(PushMember):
			{
				intrinsic = symbol == K32("++") ? kopPostInc : kopPostDec;
				last++;
				Inc(src, itr);

				CallHandler::Input input(src, scope);

				input.Add(var);

				return Assume(CallHandler::Resolve({}, intrinsic, input), SyntaxError(src, kErrorOperatorNotFound));
			}

			default:
				SyntaxError::Throw(src);
				return 0;
			}
		}
		break;

	case K32("+="):
		intrinsic = kopAddAssign;
		break;

	case K32("-="):
		intrinsic = kopSubtractAssign;
		break;

	case K32("*="):
		intrinsic = kopMultiplyAssign;
		break;

	case K32("/="):
		intrinsic = kopDivideAssign;
		break;

	case K32("*"):
		intrinsic = kopMultiply;
		break;

	case K32("/"):
		intrinsic = kopDivide;
		break;

	case K32("+"):
		intrinsic = kopAdd;
		break;

	case K32("-"):
		intrinsic = kopSubtract;
		break;

	case K32("%"):
		intrinsic = kopMod;
		break;

	case K32("&"):
		intrinsic = kopBand;
		break;

	case K32("|"):
		intrinsic = kopBor;
		break;

	case K32("<<"):
		intrinsic = kopShl;
		break;

	case K32(">>"):
		intrinsic = kopShr;
		break;

	case K32("^"):
		intrinsic = kopXor;
		break;


	//bool operators

	case K32("<"):
		intrinsic = kopLessThan;
		break;

	case K32(">"):
		intrinsic = kopGreaterThan;
		break;

	case K32("<="):
		intrinsic = kopLessThanOrEqual;
		break;

	case K32(">="):
		intrinsic = kopGreaterThanOrEqual;
		break;

	case K32("=="):
		intrinsic = kopEqual;
		break;

	case K32("!="):
		intrinsic = kopInequal;
		break;

	case K32("&&"):
		intrinsic = kopLogicalAnd;
		break;

	case K32("||"):
		intrinsic = kopLogicalOr;
		break;

	case K32("?"):
		break;

	default:
		return 0;
	}

	Inc(src, itr);

	ResolveIfObjectTemporary(src, scope, var);

	CallHandler::Input input(src, scope);

	input.AddCaller(var);

	input.Add(itr, { CallHandler::Input::kDelimiterSemiColon, CallHandler::Input::kDelimiterComma }, false);

	return Assume(CallHandler::Resolve({}, intrinsic, input), SyntaxError(src, kErrorOperatorNotFound));
}

TypeRef CompilerImpl::ExpressionHandler::Private::ApplyBind(Source & src, Scope & scope, Itr & itr, const Function & fn, Array <TypeRef> & captures)
{
	auto & state = scope.state;

	for (auto & i : fn.args) Assume(!i.byref, SyntaxError(src, "cannot bind to reference param"));

	auto ncapture = UInt16(captures.GetSize());

	if (ncapture <= fn.args.GetSize())
	{
		auto args_captures = ReverseSplice(fn.args, ncapture);

		auto b = args_captures.b.data;

		bool objects = false;

		bool threadsafe = true;

		bool noncircular = true;

		UInt16 size = 0;

		REFLEX_FOREACH(input_t, captures)
		{
			TypeRef target_t = (*b++).type;

			size += target_t->size;

			Assume(input_t == target_t, TypeError(src));

			input_t = target_t;

			objects = objects || target_t->IsObject();

			auto flags = target_t->flags;

			noncircular = noncircular && flags.Check(Type::kFlagNonCircular);

			threadsafe = threadsafe && flags.Check(Type::kFlagThreadsafe);
		}

		Array <TypeRef> signature = { fn.rtn.type };

		for (auto & i : args_captures.a) signature.Push(i.type);

		auto type = state.InstantiateTemplateType(kFn, signature);

		auto pinstructions = Extend(scope.instructions, 2 + ncapture).data;

		*pinstructions++ = { src.line, src.file, OPCODE(BindFnObject), GetContainerType2(True(captures), objects, threadsafe, noncircular), ToUIntNative(&fn) };

		*pinstructions++ = { src.line, src.file, OPCODE(Data), Reinterpret<UInt32>(MakeTuple(ncapture, size)), ToUIntNative(type) };

		//*pinstructions++ = {src.line, src.file, OPCODE(Data), 0, Reinterpret<UInt64>(MakeTuple(ncapture, size)) };

		REFLEX_LOOP_PTR(captures.GetData(), ptyperef, ncapture)
		{
			*pinstructions++ = { src.line, src.file, OPCODE(Data), 0, ToUIntNative(*ptyperef) };
		}

		return type;
	}

	UNREACHABLE_X(SyntaxError, kErrorTypeMismatch);
}

TypeRef CompilerImpl::ExpressionHandler::Private::AssumeConstructor(Source & src, Scope & scope, TypeRef type, const Token * inner)
{
	auto itr = inner;

	if (auto namedctr = CallHandler::BuildNamedConstructorInput(src, scope, inner, type))
	{
		namedctr.AddTArg(type);

		return Assume(CallHandler::ResolveOperator(ToView(type->symbol.a), kopCreate, namedctr), SyntaxError(src, kErrorMatchingConstructorNotFound));
	}
	else
	{
		CallHandler::Input input(src, scope);

		input.AddTArg(type);

		CallHandler::BuildRegularArguments(input, itr);

		auto rhs = CallHandler::ResolveOperator(ToView(type->symbol.a), kopCreate, input);

		if (rhs == type)
		{
			return type;
		}
		else if (!input.GetSlots() && IsDefaultConstructable(type))
		{
			AddPushNew(input.scope.instructions, src, type);

			return type;
		}

		SyntaxError::Throw(src, kErrorMatchingConstructorNotFound);

		return 0;
	}
}

//TypeRef CompilerImpl::ExpressionHandler::Private::AssumeConstructor(Source & src, Scope & scope, TypeRef type, const Itr inner)
//{
//	CallHandler::Input input(src, scope);
//
//	input.AddTArg(type);
//
//	auto rhs = CallHandler::ResolveOperator(type, kopCreate, input);
//
//	if (rhs == type)
//	{
//		return type;
//	}
//	else if (IsDefaultConstructable(type))
//	{
//		AddPushNew(input.scope.instructions, src, type);
//
//		return type;
//	}
//
//	SyntaxError::Throw(src, kErrorMatchingConstructorNotFound);
//
//	return 0;
//}

REFLEX_END_INTERNAL

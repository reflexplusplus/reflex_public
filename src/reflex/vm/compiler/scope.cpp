#include "compilerimpl.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

CompilerImpl::Scope::Scope(StateImpl & state, bool global, bool sub, Scope * parent, LayoutTemplate & stackframelayout, Array < Pair <Symbol, Variable> > & variables, Array <Instruction> & instructions, Key32 localns)	//base constructor for 'full' scopes (Global and Function)
	: state(state),
	allocator(state.m_allocator),
	bindings(state.bindings),

	stackframelayout(stackframelayout),
	instructions(instructions),
	variables(variables),

	m_global(global),
	m_sub(sub),
	m_parent(parent),
	m_localns(localns),

	m_usings(allocator)
{
	if (!parent)
	{
		state.root = this;

		m_prev = 0;
	}
	else
	{
		m_prev = parent;
	}

	state.current = this;

	if (parent)
	{
		m_usings = parent->m_usings;
	}

	if (IsValidKey(localns)) RegisterUsing(localns);
}

CompilerImpl::Scope::~Scope()
{
	state.current = m_parent;
}

void CompilerImpl::Scope::DispatchExitKeyword(Key32 keyword, Source & src, Itr & itr)
{
	bool done = false;

	auto scope = this;

	while (scope && !done)
	{
		if (scope->OnHandleExit(keyword, src, itr))
		{
			ExpectSemiColon(src, itr);

			return;
		}

		scope = scope->GetPrev();
	}

	SyntaxError::Throw(src, kErrorInternalError);
}

TypeRef CompilerImpl::Scope::ParseType(Source & src, Itr & itr)
{
	//TODO
	//
	//static bool Get(Scope & scope, const Pair <CString::View> & symbol, bool istemplate, const Param & param, Output & output)

	//istemplate = last_delimiter == '@'

	//Funkspace:: PropertySet @ Tits

	struct TypeAccessor
	{
		typedef CString::View SymbolType;

		typedef Tuple <Source*,Itr*> Param;

		typedef TypeRef Output;

		static bool Get(Scope & scope, const Pair <Key32,CString::View> & symbolx, const Param & param, Output & output)
		{
			if (output) return true;

			auto & state = scope.state;

			Symbol symbol = { symbolx.a, symbolx.b };

			if (auto palias = state.m_typedefs.SearchValue(ToUInt64(symbol)))
			{
				output = *palias;
			}
			else if (auto type = GetTypeBySymbol(scope.bindings, symbol))
			{
				output = type;

				//return true;
			}
			//else if (auto idx = Search(symbolx.b, '@'))
			//{
			//	auto & src = *param.a;

			//	auto subitr = *param.b;

			//	auto start = symbolx.b.a + *idx + 1;

			//	while (subitr->value.a != start)
			//	{
			//		subitr = subitr->GetPrev();
			//	}

			//	Symbol tsymbol = { symbol.a, Splice(symbolx.b, *idx).a };

			//	int a = 1;
			//}
			else if (state.GetTemplateType(symbol))
			{
				auto & src = *param.a;

				auto & subitr = *param.b;

				if (Traverse<Token::kTypeSymbol>(src, subitr, kSymbolAt))
				{
					if (auto bracketscope = EnterBrackets(src, subitr, kBracketRound))
					{
						Itr child = bracketscope.inner;

						Array <TypeRef> template_args(scope.allocator);

						while (auto type = scope.ParseType(src, child))
						{
							template_args.Push(type);

							if (child)
							{
								Expect<Token::kTypeSymbol>(src, child, kSymbolComma);
							}
							else
							{
								break;
							}
						}

						Assume(!child, SyntaxError(src, kErrorExpectedType));

						output = state.InstantiateTemplateType(symbol, template_args);

						//output = ToPointer<Detail::Type>(kMaxUIntNative);
					}
					else
					{
						auto arg = ExpectType(src, subitr, scope);

						output = state.InstantiateTemplateType(symbol, ToView(arg));

						//output = ToPointer<Detail::Type>(kMaxUIntNative);
					}

					//return true;
				}
			}

			return true;
		}
	};

	if (Peek(src, itr, Token::kTypeWord))
	{
		auto store = itr;

		if (auto type = TraverseSymbolsOfType<TypeAccessor>(src, itr, { &src, &itr }))
		{
			return type;
		}
		else if (store->hash == ktypeof)
		{
			Inc(src, itr);

			InstructionsRestorePoint restore(*this);

			Itr inner = Assume(BracketScope(src, itr, kBracketRound).inner, SyntaxError(src, kErrorExpectedBrackets));

			auto type = Assume(ExpressionHandler::Parse(src, inner, *this, ExpressionHandler::kContextStatement, 0), SyntaxError(src, kErrorExpectedStatement));

			restore.Rollback();

			if (auto bracketscope = BracketScope(src, itr, kBracketSquare))
			{
				Itr subtype = bracketscope.inner;

				auto number = UInt8(ToUInt32(Assume(Traverse<Token::kTypeInt>(src, subtype), SyntaxError(src))->value));

				auto & targs = Assume(state.GetInstantiatedTemplateType(type), SyntaxError(src))->targs;

				Assume(number < targs.GetSize() && !subtype, SyntaxError(src));

				return targs[number];
			}

			return type;
		}

		itr = store;
	}
	else if (auto p = Peek(src, itr, Token::kTypeSymbol))
	{
		if (p->hash == kSymbolNamespaceDelimiter)
		{
			Inc(src, itr);

			SubScope subscope(*this);

			subscope.RegisterUseGlobal();

			return subscope.ParseType(src, itr);
		}
		else
		{
			return 0;
		}
	}

	return 0;
}

CString::View CompilerImpl::Scope::ResolveNamespace(Source & src, const CString::View & value) const
{
	auto pair = SplitNamespacedSymbol(value);

	auto & word = pair.b;

	if (auto & ns = pair.a)
	{
		REFLEX_RFOREACH(i, m_usings)
		{
			if (auto string = GetNamespaceString(state, i))
			{
				Key32 t = Join(string, kNamespaceDelimiter, ns, kNamespaceDelimiter, word);

				if (auto full = GetNamespaceString(state, t)) return full;
			}
		}

		Key32 t = Join(ns, kNamespaceDelimiter, word);

		if (auto full = GetNamespaceString(state, t)) return full;
	}
	else
	{
		REFLEX_RFOREACH(i, m_usings)
		{
			if (auto string = GetNamespaceString(state, i))	//could be anon,
			{
				Key32 t = Join(string, kNamespaceDelimiter, word);

				if (auto full = GetNamespaceString(state, t)) return full;
			}
		}

		if (auto full = GetNamespaceString(state, word)) return full;
	}

	SyntaxError::Throw(src, kErrorExpectedNamespace);

	return {};
}

#define VM_PTRSIZE sizeof(void*)
#define VM_MODULO_PTRSIZE(a) (a & (sizeof(void*)-1))

const Variable & CompilerImpl::Scope::RegisterVariable(const Source & src, Key32 name, TypeRef type)
{
	INLINE(Variable,AquireVariable)(Scope & scope, const Source & src, Key32 name, const Type & type, LayoutTemplate & layout)
	{
		auto isglobal = scope.IsGlobal();

		UInt16 offset = 0;

		if (type.IsObject())
		{
			offset = layout.AddObject<true>(type, name, &REFLEX_NULL(Object));
		}
		else
		{
			REFLEX_ASSERT(type.params.GetSize() == type.size);

			offset = layout.AddValue(type, name, type.params);
		}

		return { &type, Int16(offset), isglobal ? Location::kGlobal : Location::kLocal, false };
	}
	END

	Symbol symbol = { m_localns, name };

	Assume(!Search<KeyCompare>(variables, symbol), SyntaxError(src, kErrorDuplicateSymbol));

	auto var = AquireVariable::Call(*this, src, name, *type, stackframelayout);

	return variables.Push({ symbol, var }).b;
}

CompilerImpl::SubScope::SubScope(Scope & parent, Key32 ns)
	: Scope(parent.state, parent.IsGlobal(), true, &parent, parent.stackframelayout, parent.variables, parent.instructions, ns)
{
}

CompilerImpl::SubScope::SubScope(Scope & parent)
	: SubScope(parent, parent.GetNamespace())
{
}

bool CompilerImpl::SubScope::OnHandleExit(Key32 keyword, Source & src, Itr & itr)
{
	return false;
}

CompilerImpl::AnonScope::AnonScope(Scope & parent)
	: SubScope(parent, parent.state.AcquireAnonSymbol())
{
}

CompilerImpl::NamespaceScope::NamespaceScope(Scope & parent, const CString::View & ns)
	: SubScope(parent, [&parent, &ns]() -> Key32
	{
		auto & state = parent.state;

		if (auto string = GetNamespaceString(state, parent.GetNamespace()))
		{
			return AcquireStaticString(state, Join(string, kNamespaceDelimiter, ns));
		}
		else
		{
			return AcquireStaticString(state, ns);
		}
	}())
{
}

template <class TYPE> REFLEX_INLINE CompilerImpl::Scope::SymbolRef MakeSymbolRef(CompilerImpl::Scope::SymbolType type, const TYPE & value)
{
	CompilerImpl::Scope::SymbolRef output;

	output.a = type;

	Reinterpret<TYPE>(output.b) = value;

	return output;
}

bool CompilerImpl::Scope::SymbolAccessor::Get(Scope & scope, Symbol symbol, const Pair <Scope*> & scopes, Output & output)
{
	auto & state = scope.state;

	if (auto var = scopes.a->SearchVariable(symbol))
	{
		output = MakeSymbolRef(kSymbolTypeVariable, var);

		return false;
	}
	else if (scopes.b)
	{
		if (auto var = scopes.b->SearchVariable(symbol))
		{
			output = MakeSymbolRef(kSymbolTypeVariable, var);

			return false;
		}
	}

	if (auto pconstant = state.GetConstValue(symbol))
	{
		output = MakeSymbolRef(kSymbolTypeConstant, pconstant);

		return false;
	}

	return true;
}

REFLEX_END_INTERNAL

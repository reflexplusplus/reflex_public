#include "compilerimpl.h"

#include "../library.h"

#include "tokeniser.h"
#include "scope.h"
#include "functionhandler.h"
#include "expressionhandler.h"
#include "callhandler.h"

#include "../bindings/core/array.h"



//
//

REFLEX_NS(Reflex::VM)

const CString::View kErrorStageParse = "parse";
const CString::View kErrorStageCompile = "compile";
const CString::View kErrorStageLink = "link";
const CString::View kErrorStageRuntime = "runtime";

Output output("VM");	//, Debug::kOutputQueue | Debug::kOutputConsole);

constexpr CString::View kAutos[] = { "AUTO1", "AUTO2", "AUTO3", "AUTO4", "AUTO5", "AUTO6", "AUTO7", "AUTO8" };

REFLEX_END




REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

bool InstantiateModuleGroup(Compiler::State & state, const CString::View & id)
{
	//TODO optimise with map

	bool found = false;

	auto length = id.size;

	for (auto & i : Module::range)
	{
		if (Left<true>(i.name, length) == id)
		{
			state.Instantiate(i);

			found = true;
		}
	}

	return found;
}

const Pair <decltype(&CompilerImpl::ExpressionHandler::CanCast), decltype(&CompilerImpl::ExpressionHandler::ExpectCast)> kNoCast =
{
	[](CompilerImpl::Source & src, CompilerImpl::Scope &, const Variable & from, TypeRef to, UInt &)
	{
		return true;
	},
	[](CompilerImpl::Source & src, CompilerImpl::Scope &, const Variable & from, TypeRef to, UInt &)
	{
		return from;
	}
};

struct CompilerImpl::SourceFileImpl : public Data::ArchiveObject
{
	using Data::ArchiveObject::ArchiveObject;

	void OnUnsetProperty(Address adr) override
	{
		if (adr == MakeAddress<Data::PropertySet>(Data::kError))
		{
			m_error.Clear();
		}
	}

	void OnSetProperty(Address adr, Object & object) override
	{
		if (adr == MakeAddress<Data::PropertySet>(Data::kError))
		{
			m_error = Cast<Data::PropertySet>(object);
		}
	}

	void OnQueryProperty(Address adr, Object * & ptr) const override
	{
		if (adr == MakeAddress<Data::PropertySet>(Data::kError))
		{
			ptr = m_error.Adr();
		}
	}

	mutable Reference <Data::PropertySet> m_error;
};

void CompilerImpl::StateImpl::RegisterResourceType(Key32 id, TypeRef type, File::ResourcePool::Ctr ctr, const WString::View & extension, ConstTRef <Data::PropertySet> options)
{
	m_resourcetypes.Push({ id, type, ctr, extension, options });
}

TRef <CompilerImpl::StateImpl> CompilerImpl::StateImpl::Clone(UInt8 context_flags, Object & client) const
{
	if (bindings->context_flags == context_flags)
	{
		return REFLEX_CREATE(StateImpl, *this, client);
	}
	else
	{
		return REFLEX_CREATE(StateImpl, *REFLEX_CREATE(Bindings, context_flags), client);
	}
}

CompilerImpl::StateImpl::StateImpl(Bindings & bindings, Object & client)
	: CloneableState(bindings),
	client(client),
	m_allocator(CreateAllocator(kBlockAllocID, REFLEX_NULL(Object))),
	scriptfunction_layouts(*m_allocator),
	script_ttypes(*m_allocator),
	script_tfns(*m_allocator),
	symbol_cache(*m_allocator),
	arguments_cache(*m_allocator),
	constant_cache(*m_allocator),
	inheritancechains_cache(*m_allocator),
	instantiated_tfn_guard(*m_allocator),
	castable_cache(*m_allocator),
	stringpool(Data::Detail::AcquireProperty<StringPool>(bindings, K32("stringpool")))
{
	m_tempsymbols.Insert(kNullKey);

	RegisterTemplateType(kGlobal, "Fn", 7, true, {}, [](State & state, const ClientData clientdata, Key32 ns, const CString::View & name, const ArrayView<TypeRef> & args)
	{
		return FnObject::RegisterType(state.bindings, name, args);
	});

	stringpool.value.Allocate(32);

	m_typedefs.Insert(ToUInt64({ kGlobal, K32("bool") }), bindings.uint8_t);

	m_auto_placeholder = CreateValueType(bindings, 0, kGlobal, "anon_auto", {});

	REFLEX_LOOP(idx, GetArraySize(m_targ_placeholders)) m_targ_placeholders[idx] = CreateValueType(bindings, kMaxUInt32 - idx, kGlobal, kAutos[idx], {});
}

CompilerImpl::StateImpl::StateImpl(const StateImpl & value, Object & client)
	: CloneableState(*REFLEX_CREATE(Bindings, value.bindings)),
	m_compiler(value.m_compiler),
	client(client),
	m_modules(value.m_modules),
	m_resourcetypes(value.m_resourcetypes),
	m_tempsymbols(value.m_tempsymbols),
	m_constants(value.m_constants),
	m_template_types(value.m_template_types),
	m_template_functions(value.m_template_functions),
	m_instantiated_template_types(value.m_instantiated_template_types),
	m_typedefs(value.m_typedefs),

	//resourcepool(resourcepool),
	m_allocator(CreateAllocator(kBlockAllocID, REFLEX_NULL(Object))),
	scriptfunction_layouts(*m_allocator),
	script_ttypes(*m_allocator),
	script_tfns(*m_allocator),
	symbol_cache(*m_allocator),
	arguments_cache(*m_allocator),
	constant_cache(*m_allocator),
	inheritancechains_cache(*m_allocator),
	instantiated_tfn_guard(*m_allocator),
	castable_cache(*m_allocator),
	stringpool(value.stringpool)
{
	bindings->SetProperty(K32("stringpool"), stringpool);

	m_auto_placeholder = GetTypeBySymbol(bindings, { kGlobal, K32("anon_auto") });

	REFLEX_LOOP(idx, GetArraySize(m_targ_placeholders)) m_targ_placeholders[idx] = GetTypeBySymbol(bindings, {kGlobal, kAutos[idx]});
}

bool CompilerImpl::StateImpl::Instantiate(const Module & module)
{
	if (Search(m_modules, module.id))
	{
		return true;
	}
	else
	{
		auto context_flags = bindings->context_flags;

		if (module.context_flags & context_flags)
		{
			for (auto & i : module.dependencies) Instantiate(i);

			REFLEX_IF_DEBUG(UInt n = m_modules.GetSize());

			{
				ScopeTimer timer(output, module.name);

				module.instantiator(*this, context_flags, client);
			}

			REFLEX_IF_DEBUG(REFLEX_ASSERT(!SetDelta(n, m_modules.GetSize())));

			m_modules.Push(module.id);

			return true;
		}
	}

	return false;
}

Symbol CompilerImpl::StateImpl::RegisterConstant(TypeRef typeref, Key32 ns, const CString::View & name, const Data::Archive::View & value)
{
	auto size = typeref->size;

	REFLEX_ASSERT(size <= 16 && size == value.size);

	Symbol symbol = { ns, name };

	auto & constant = m_constants.Acquire(ToUInt64(symbol));

	constant.symbol = symbol;
	constant.name = name;
	constant.type = typeref;

	MemCopy(value.data, &constant.value, size);

	return symbol;
}

Symbol CompilerImpl::StateImpl::RegisterTemplateType(Key32 ns, const CString::View & name, UInt ntarg, bool varadic, const Data::Archive::View & clientdata, TemplateType::Instantiator callback)
{
	REFLEX_ASSERT(clientdata.size < sizeof(ClientData));

	Symbol symbol = { ns, name };

	auto & template_type = m_template_types.Acquire(ToUInt64(symbol));

	template_type.symbol = symbol;
	template_type.name = name;
	template_type.ntarg = UInt8(ntarg);
	template_type.varadic = varadic;
	MemCopy(clientdata.data, template_type.clientdata, clientdata.size);
	template_type.instantiate = callback;

	return symbol;
}

const Compiler::State::InstantiatedTemplateType * CompilerImpl::StateImpl::GetInstantiatedTemplateType(TypeRef type) const
{
	return m_instantiated_template_types.SearchValue(type);
}

Symbol CompilerImpl::StateImpl::RegisterTemplateFunction(Key32 ns, const CString::View & name, UInt32 ntarg, const Array<Argument>::View & args, TemplateFunction::Instantiator callback, UInt8 flags)
{
	Symbol symbol = { ns, name };

	auto & fn = m_template_functions.Insert(ToUInt64(symbol));

	fn.symbol = symbol;
	fn.name = name;
	fn.flags = flags;
	fn.ntarg = ntarg;
	fn.arguments = args;
	fn.instantiate = callback;

	return symbol;
}

TypeRef CompilerImpl::StateImpl::InstantiateTemplateType(Symbol tsymbol, const ArrayView <TypeRef> & targs)
{
	if (auto ptemplate = m_template_types.SearchValue(ToUInt64(tsymbol)))
	{
		auto & template_t = (*ptemplate);

		auto ntarg = UInt8(targs.size);

		if (ntarg == template_t.ntarg || (template_t.varadic && ntarg && ntarg <= template_t.ntarg))
		{
			auto name = MakeTemplateName(*this, template_t.name, targs);

			Symbol symbol = { tsymbol.a, name };

			if (auto type = GetTypeBySymbol(bindings, symbol))
			{
				m_instantiated_template_types.Acquire(type) = { symbol, type, targs };	//this allows c++ 'override' types to be defined first

				return type;
			}
			else
			{
				if (auto type = template_t.instantiate(*this, ptemplate->clientdata, symbol.a, AcquireStaticString(*this, std::move(name)), targs))
				{
					m_instantiated_template_types.Acquire(type) = { symbol, type, targs };

					return type;
				}
			}
		}
	}

	return 0;
}

TRef <Compiler::State> CompilerImpl::CreateState(UInt8 contextflags) const
{
	return REFLEX_CREATE(StateImpl, *REFLEX_CREATE(Bindings, contextflags), REFLEX_NULL(Object));
}

TRef <Program> CompilerImpl::Compile(File::ResourcePool::Lock & lock, const WString::View & source, UInt8 contextflags, Object & client, const ArrayView <ConstTRef<Module>> & defaultmodules, const State & prebinding) const
{
	ScopeTimer scope(output, Join("Compile ", ToCString(File::SplitFilename(source).b)));

	auto state = AutoRelease(Cast<CloneableState>(prebinding)->Clone(contextflags, client));

	StateImpl::ScopeOf stateof(state.Adr());	//for Program

	auto target = New<ProgramImpl>(state->bindings);

	state->m_compiler = this;

	state->m_target = target;

	for (auto & i : defaultmodules) state->Instantiate(i);

	//auto & error = state->error_final;

	try
	{
		try
		{
			struct GlobalScope : public Scope
			{
				GlobalScope(StateImpl & state, ProgramImpl & target)
					: Scope(state, true, false, 0, layout, variables, target.global_instructions, kNullKey),
					variables(allocator)
				{
					m_usings.Push(kNullKey);
				}

				virtual bool OnHandleExit(Key32 keyword, Source & src, Itr & itr) override
				{
					if (keyword == kexit)
					{
						AddAbort(instructions, src);

						return true;
					}

					return Scope::OnHandleExit(keyword, src, itr);
				}

				LayoutTemplate layout;

				Array < Pair <Symbol,Variable> > variables;
			}

			global(state, target);

			state->filesystemlock = &lock;

			if (!state->Compile(source, global)) output.Error(source, kErrorPathNotFound);

			REFLEX_ASSERT(state->current == &global);

			if (kOptimise) target->Optimise(state, global.layout, global.instructions, state->scriptfunction_layouts);

			target->Link(state, global.layout, state->scriptfunction_layouts);

			if (REFLEX_DEBUG)
			{
				target->List(state, global.layout, state->scriptfunction_layouts);
			}
		}
		catch (const SyntaxError & syntaxerror)
		{
			//error = syntaxerror.error;

			Error e(syntaxerror.src, kErrorStageCompile, syntaxerror.error);

			throw(e);
		}
		catch (const TypeError & typeerror)
		{
			//errors.Push(typeerror.error);

			Error e(typeerror.src, kErrorStageCompile, typeerror.error);

			throw(e);
		}
	}
	catch (const Error & e)
	{
		target->Invalidate();

		PublishError(lock.resourcepool, target->sources[e.source.file].pathview, e.source.line, std::move(RemoveConst(e).stage), std::move(RemoveConst(e).msg));
	}

	return target;
}

Data::ArchiveObject * CompilerImpl::StateImpl::ParseSource(const WString::View & path, const File::ResourcePool::Token * & ptoken)
{
	auto archive_rttid = REFLEX_TYPEID(Data::ArchiveObject);

	auto & res = filesystemlock->RetrieveToken(archive_rttid, path, {}, &OpenSourceFile);

	auto source = Cast<Data::ArchiveObject>(res.object);

	if (SetFiltered(includeguard.Acquire(source.Adr()), true))
	{
		auto & resolved_path = res.attributes.resolved_path;

		if (resolved_path != res.path) output.Warn(res.path, "resolved to", resolved_path);

		auto & vmpath = AquireStringLiteral(res.path, Copy(res.path));

		AddSource(m_target, vmpath, archive_rttid, source);

		ptoken = &res;

		return source.Adr();
	}

	return 0;
}

bool CompilerImpl::StateImpl::Compile(const WString::View & path, Scope & global)
{
	auto & sources = m_target->sources;

	const File::ResourcePool::Token * pres;// = RetrieveSource(path);

	if (auto psource = ParseSource(path, pres))
	{
		auto fileidx = UInt8(sources.GetSize() - 1);

		currentpath = File::SplitFilename(pres->path);

		Data::ClearError(*psource);

		try
		{
			auto token = Tokenise(Data::Unpack<CString::View>(psource->value));

			Source src(token, fileidx, 0);	//FIX

			Itr itr = token->GetFirst();

			REFLEX_ASSERT(global.IsGlobal());

			ParseScope(src, itr, global);
		}
		catch (Pair<UInt,CString> & e)
		{
			throw(Error({ UInt16(e.a), fileidx }, kErrorStageParse, std::move(e.b)));
		}
		catch (Error & e)
		{
			throw(e);
		}

		return !File::IsMissing(pres->attributes.status);
	}

	return true;
}

REFLEX_INLINE const Token & CompilerImpl::Inc(Source & src, Itr & token)
{
	auto & rtn = *token;

	src.line = UInt16(rtn.linenumber);

	token = token->GetNext();

	return rtn;
}

template <Token::Type TYPE> REFLEX_INLINE const Token * CompilerImpl::Traverse(Source & src, Itr & token, Key32 hash)
{
	REFLEX_STATIC_ASSERT(TYPE != Token::kTypeNull);

	if (token)
	{
		if constexpr (((TYPE == Token::kTypeBracket) || (TYPE == Token::kTypeKeyword) || (TYPE == Token::kTypeSymbol)))
		{
			REFLEX_ASSERT(IsValidKey(hash));

			if (token->hash == hash)
			{
				REFLEX_ASSERT(token->type == TYPE);

				return &Inc(src, token);
			}
			else
			{
				return 0;
			}
		}
		else if (token->type == TYPE)
		{
			REFLEX_ASSERT(IsNullKey(hash));

			return &Inc(src, token);
		}
	}

	return 0;
}

REFLEX_INLINE const Token * CompilerImpl::TraverseWord(Source & src, Itr & token, Key32 value)
{
	if (token)
	{
		if (/*token->type == Token::kTypeWord && */token->hash == value)
		{
			REFLEX_ASSERT(token->type == Token::kTypeWord);

			return &Inc(src, token);
		}
	}

	return 0;
}

template <class TYPE>
struct OutputInitialiser
{
	static TYPE Call(CompilerImpl::Scope &)
	{
		return {};
	}
};

template <class TYPE>
struct OutputInitialiser < Array <TYPE> >
{
	static Array <TYPE> Call(CompilerImpl::Scope & scope)
	{
		Array <TYPE> t(scope.allocator);

		t.Allocate(8);

		return t;
	}
};

template <class ACCESSOR> auto CompilerImpl::GetSymbolsOfType(Scope & scope, const CString::View & ns, typename ACCESSOR::SymbolType symbol, const ArrayView <Key32> & usings, const typename ACCESSOR::Param & param)
{
	typedef typename ACCESSOR::Output Output;

	Output candidates = OutputInitialiser<Output>::Call(scope);

	if (ns)
	{
		auto & state = scope.state;

		REFLEX_RFOREACH(i, usings)
		{
			if (auto string = GetNamespaceString(state, i))
			{
				auto full = Reflex::Detail::Joiner::Join(scope.allocator, string, kNamespaceDelimiter, ns);

				if (!ACCESSOR::Get(scope, { full, symbol }, param, candidates)) return candidates;
			}
		}

		ACCESSOR::Get(scope, { ns, symbol }, param, candidates);
	}
	else
	{
		REFLEX_RFOREACH(i, usings)
		{
			if (!ACCESSOR::Get(scope, { i, symbol }, param, candidates)) return candidates;
		}
	}

	return candidates;
}

REFLEX_INLINE CompilerImpl::BracketScope::BracketScope(Source & src, Itr & itr, Key32 value)
	: src(src),
	m_token(Traverse<Token::kTypeBracket>(src, itr, value)),
	inner(0),
	line(src.line)
{
	if (m_token)
	{
		inner = m_token->GetFirst();
	}
}

REFLEX_INLINE CompilerImpl::BracketScope::~BracketScope()
{
	src.line = line;
}

REFLEX_INLINE CompilerImpl::BracketScope CompilerImpl::EnterBrackets(Source & src, Itr & itr, Key32 value, Itr & rtn)
{
	BracketScope t(src, itr, value);

	rtn = t.inner;

	return t;
}

template <Token::Type TYPE> REFLEX_INLINE CompilerImpl::Itr CompilerImpl::Expect(Source & src, Itr & token, Key32 hash)
{
	return Assume(Traverse<TYPE>(src, token, hash), SyntaxError(src));
}

REFLEX_INLINE void CompilerImpl::ExpectSemiColon(Source & src, Itr & token)
{
	Assume(Traverse<Token::kTypeSymbol>(src, token, kSymbolSemiColon), SyntaxError(src, kErrorExpectedSemiColon));
}

REFLEX_INLINE void CompilerImpl::DispatchPreprocessor(Source & src, Itr & itr, Scope & global)
{
	LOCAL(WString, TraversePath)(StateImpl & state, Source & src, Itr & itr)
	{
		Source dummy(*itr, 0, 0);

		auto string = Assume(Expect<Token::kTypeDoubleQuotedString>(dummy, itr)->value, SyntaxError(src));

		return File::ResolveIncludePath(state.currentpath.a, ToWString(string));
	}
	END

	constexpr auto ParseInclude = [](StateImpl & state, Scope & global, Source & src, Itr & itr)
	{
		auto current = state.currentpath;

		Assume(state.Compile(TraversePath::Call(state, src, itr), global), SyntaxError(src, kErrorPathNotFound));

		state.currentpath = current;
	};

	constexpr auto ParseModule = [](StateImpl & state, Scope & global, Source & src, Itr & itr)
	{
		auto value = Assume(Traverse<Token::kTypeDoubleQuotedString>(src, itr), SyntaxError(src));

		Assume(InstantiateModuleGroup(global.state, value->value), SyntaxError(src, kErrorPathNotFound));
	};

	constexpr auto ParseResource = [](StateImpl & state, Scope & global, Source & src, Itr & itr)
	{
		auto brackets = EnterBrackets(src, itr, kBracketRound);

		auto path = TraversePath::Call(state, src, itr);

		Pair <CString::View> symbol;

		if (auto as = TraverseWord(src, itr, K32("as")))
		{
			symbol = ParseNamespacedSymbol(src, itr);
		}

		try
		{
			if (brackets.inner)
			{
				auto id = brackets.inner->hash;

				for (auto & i : state.m_resourcetypes)
				{
					if (i.a == id)
					{
						throw(i);
					}
				}
			}
			else
			{
				auto extension = File::SplitExtension(path).b;

				for (auto & i : state.m_resourcetypes)
				{
					if (i.d == extension)
					{
						throw(i);
					}
				}
			}
		}
		catch (const decltype(state.m_resourcetypes)::Type & i)
		{
			//auto param = Join(Data::Pack(MakeTuple(state.m_compiler.Adr(), state.m_target.Adr(), state.filesystemlock)), i.e);

			auto & token = state.filesystemlock->RetrieveToken(i.b->type_id, path, i.e, i.c);

			auto vmpath = New<String>(path);

			AddSource(state.m_target, *vmpath, i.b->type_id, token.object);

			Assume(!File::IsMissing(token.attributes.status), SyntaxError(src, kErrorPathNotFound));

			if (symbol.b) state.RegisterConstant(i.b, symbol.a, symbol.b, Data::Pack(token.object.Adr()));

			return;
		}

		SyntaxError::Throw(src, kErrorResourceHandlerNotFound);
	};

	auto & state = global.state;

	auto symbol = Assume(Traverse<Token::kTypeWord>(src, itr), SyntaxError(src, kErrorExpectedPreprocessorCommand))->hash;

	switch (symbol.value)
	{
	case K32("include"):
		ParseInclude(state, global, src, itr);
		break;

	case K32("module"):
		ParseModule(state, global, src, itr);
		break;

	case K32("resource"):
		ParseResource(state, global, src, itr);
		break;

	case K32("polyfill"):
		break;
	}
}

void CompilerImpl::DispatchForeach(Source & src, Itr & itr, Scope & scope)
{
	struct ForeachScope : public AnonScope
	{
		ForeachScope(const Source & src, Scope & parent, Argument itr_arg, const ArrayView <Argument> & arguments, const Function & begin, const Function & next)
			: AnonScope(parent),
			src(src)
		{
			auto & itr = RemoveConst(RegisterVariable(src, state.AcquireAnonSymbol(), itr_arg.type));



			//opBegin

			AddCall(instructions, src, begin, begin.args.GetSize());

			AddAssignVariable(instructions, src, itr);



			//loop

			m_continue = state.AcquireAnonSymbol();

			m_done = state.AcquireAnonSymbol();

			AddMarker(instructions, src, m_continue);

			AddPushVariable(instructions, src, itr, itr_arg.byref);

			for (auto & i : arguments)
			{
				auto & value = RemoveConst(RegisterVariable(src, state.GetStaticString(i.name, false), i.type));

				value.is_const = true;

				AddPushVariable(instructions, src, value, i.byref);
			}

			AddCall(instructions, src, next, next.args.GetSize());

			AddConditionalJump(instructions, src, m_done , 1, OPCODE(JumpIfFalse8), OPCODE(JumpIfFalse32));
		}

		~ForeachScope() override
		{
			AddJump(instructions, src, m_continue);

			AddMarker(instructions, src, m_done);
		}

		//virtual void OnReturn(Source & src, Itr & itr) override
		//{
		//	SyntaxError::Throw(src, kErrorInvalidKeywordHere);
		//}

		//virtual void OnBreak(Source & src, Itr & itr) override
		//{
		//	SyntaxError::Throw(src, kErrorInvalidKeywordHere);
		//}

		const Source & src;

		Key32 m_continue, m_done;
	};

	auto & bindings = scope.bindings;

	Argument targ = 0;

	ArrayView <Argument> targs(&targ, 0);

	CallHandler::Input input(src, scope);

	CallHandler::ParseTArgs(src, itr, input);

	if (auto bracketscope = EnterBrackets(src, itr, kBracketRound))
	{
		auto brackets = bracketscope.inner;

		Array <Argument> arguments(scope.allocator);

		do
		{
			auto token = Assume(Traverse<Token::kTypeWord>(src, brackets), SyntaxError(src, kErrorExpectedBrackets));

			AcquireStaticString(scope.state, token->value);

			arguments.Push({ 0, true, token->hash });
		}
		while (Traverse<Token::kTypeSymbol>(src, brackets, kSymbolComma));

		Assume(Traverse<Token::kTypeSymbol>(src, brackets, kSymbolColon), SyntaxError(src, kErrorExpectedColon));

		if (auto object_var = ExpressionHandler::ParseX(src, brackets, scope, ExpressionHandler::kContextTemporary, 0))
		{
			input.AddCaller(object_var);

			auto object_t = object_var.type;

			if (input.GetTArgs().GetSize() == 1)
			{
				//DYNAMIC hack instantiate PropertySet::Iterator@TYPE (proper solution is to CallHandler handles templates)

				TypeRef type = input.GetTArgs().GetFirst().type;// targs.GetFirst().type;

				scope.state.InstantiateTemplateType({ object_t->name,K32("Iterator") }, ArrayView<TypeRef>(&type, 1));
			}

			if (auto begins = CallHandler::Lookup({}, Compiler::kopBegin, input))
			{
				for (auto & i : begins)
				{
					auto & begin = *i;

					auto iterator_t = begin.rtn.type;

					input.ClearTArgs();

					input.ClearArgs();

					Variable iterator = { iterator_t, 0, Location::kConst };	//WORKAROUND FOR ASSERT, resolved later

					input.AddCaller(iterator);

					input.GetSlots().GetFirst().variable.location = Location::kTemporary;

					REFLEX_LOOP(i, arguments.GetSize()) input.Add({ scope.bindings.void_t });

					if (auto candidates = CallHandler::Lookup({}, Compiler::kopNext, input))
					{
						for (auto & i : candidates)
						{
							auto & fn = *i;

							if (fn.rtn.type == bindings.bool_t)
							{
								Assume(fn.args.GetSize() == arguments.GetSize() + 1, SyntaxError(src, kErrorInternalError));

								auto iterator_arg = fn.args[0];

								auto container_t = begin.args.GetFirst().type;

								auto castfns = kNoCast;

								if (object_t != container_t)
								{
									castfns = { &ExpressionHandler::CanCast, &ExpressionHandler::ExpectCast };
								}

								if (castfns.a(src, scope, object_var, container_t, ExpressionHandler::whatever))
								{
									if (iterator_arg.type == iterator_t)
									{
										auto args = Splice(fn.args, 1).b;

										REFLEX_LOOP(idx, arguments.GetSize()) arguments[idx].type = args[idx].type;

										auto container_t = begin.args.GetFirst().type;

										castfns.b(src, scope, object_var, container_t, ExpressionHandler::whatever);

										ForeachScope foreach(src, scope, iterator_arg, arguments, begin, fn);

										ExpectScopedStatement<DummyScope>(src, itr, foreach);

										return;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	SyntaxError::Throw(src, kErrorOperatorNotFound);
}

template <bool FOR_LOOP> void CompilerImpl::ExpectConditionalLoopInner(Source & src, AnonScope & outer, Itr & test, Itr & itr)
{
	//jump 'start'
	//marker: 'cont'
	//{code}
	//marker: 'start'
	//(test statement)
	//jnz cont

	auto & state = outer.state;

	auto & instructions = outer.instructions;

	auto start = state.AcquireAnonSymbol();

	auto cont = state.AcquireAnonSymbol();

	AddJump(instructions, src, start);

	AddMarker(instructions, src, cont);

	ExpectScopedStatement<DummyScope>(src, itr, outer);

	if constexpr (FOR_LOOP)
	{
		auto postop = test->GetNext();

		while (postop)
		{
			if (postop->hash == kSymbolSemiColon) break;

			postop = postop->GetNext();
		}

		ExpectSemiColon(src, postop);

		if (postop)
		{
			ExpressionHandler::Parse(src, postop, outer, ExpressionHandler::kContextStatement, outer.bindings.void_t);

			Assume(!postop, SyntaxError(src));
		}
	}

	AddMarker(instructions, src, start);

	auto var = ExpectConditionalExpression(src, test, outer, ExpressionHandler::kContextConditional);

	AddConditionalJump(instructions, src, cont, var.type->size, OPCODE(JumpIfTrue8), OPCODE(JumpIfTrue32));

	if constexpr (!FOR_LOOP)
	{
		Assume(!test, SyntaxError(src));
	}
}

void CompilerImpl::DispatchTemplate(Source & src, Itr & itr, Scope & scope)
{
	struct TokenCompare { static bool eq(const Token * token, Key32 hash) { return token->hash == hash; } };

	auto InstantiateType = [](State & istate, const State::ClientData clientdata, Key32 ns, const CString::View & name, const ArrayView <TypeRef> & targs) -> TypeRef
	{
		auto state = Cast<StateImpl>(istate);

		auto tname = Splice(name, Search(name, '@').value).a;

		Symbol tsymbol = { ns, tname };

		if (auto ttype = state->script_ttypes.SearchValue(ToUInt64(tsymbol)))
		{
			auto & targnames = ttype->targs;

			if (targs.size == targnames.GetSize())
			{
				Itr decl = Copy(ttype->decl);

				Source src(*ttype->decl, ttype->file, UInt16(decl->linenumber));

				auto currentpath = state->currentpath;

				state->currentpath = File::SplitFilename(state->m_target->sources[ttype->file].pathview);


				AnonScope implscope(*state->current);

				auto ptarg = targs.data;

				for (auto & i : targnames) implscope.RegisterTypedef(i, *ptarg++);

				auto ParseClass = [](Source & src, Scope & scope, Itr & itr, Key32 ns, CString::View name)
				{
					switch (itr->hash.value)
					{
					case kstruct:
						Inc(src, itr);
						return DispatchClass(src, itr, scope, ns, false, name);

					case kobject:
						Inc(src, itr);
						return DispatchClass(src, itr, scope, ns, true, name);
					}

					return TypeRef(0);
				};

				auto type = Assume(ParseClass(src, implscope, decl, ns, name), SyntaxError(src, kErrorInternalError));

				implscope.RegisterTypedef(tsymbol.b, type);

				for (auto & i : ttype->methods)
				{
					auto impl = Copy(i);

					Assume(type->IsObject(), SyntaxError(src, kErrorStructsCanNotHaveMethods));

					if (!FunctionHandler::ParseMethod(src, impl, implscope, type))
					{
						Assume(TraverseWord(src, impl, tsymbol.b), SyntaxError(src));

						FunctionHandler::AssumeConstructor(src, impl, implscope, type);
					}
				}


				state->currentpath = currentpath;

				return type;
			}
		}

		return 0;
	};

	INLINE(bool, InstantiateTemplateFunction)(State & istate, Key32 ns, const CString::View & name, const ArrayView <Argument> & targs, const ArrayView <Argument> & args)
	{
		auto state = Cast<StateImpl>(istate);

		auto symbol = ToUInt64(ns, name);

		if (auto range = MakeRange(state->script_tfns, symbol, symbol + 1))
		{
			//TODO instantiate in correct namespace

			for (auto & i : range)
			{
				auto & tfn = i.value;

				if (targs.size == tfn.targs.GetSize() && args.size == tfn.arguments.GetSize())
				{
					//HACK TO global namespace, restore current scope is error prone, note brackets

					auto current = state->current;

					//auto & global = *state.scopes.GetFirst();
					auto & global = *state->root;

					{
						AnonScope implscope(global);

						auto pargs = args.data;

						auto pautos = kAutos;

						auto ptargs = targs.data;

						for (auto & i : tfn.targs)
						{
							implscope.RegisterTypedef(i, ptargs->type);
						}

						REFLEX_LOOP(idx, tfn.arguments.GetSize())
						{
							auto arg = *pargs++;

							if (!tfn.arguments[idx].type)
							{
								if (auto explicit_idx = tfn.explicit_arguments[idx])
								{
									//auto a = 1;
									//implscope.RegisterTypedef(*pautos++, arg.type);
								}
								else
								{
									implscope.RegisterTypedef(*pautos++, arg.type);
								}
							}
						}

						Itr decl = Copy(tfn.decl);

						Source src(*tfn.decl, tfn.file, UInt16(decl->linenumber));

						FunctionHandler::ParseFunctionDeclaration(src, decl, implscope, tfn.ns);
					}

					state->current = current;

					return true;
				}
			}
		}

		return false;
	}
	END

	auto & state = scope.state;

	Array <Key32> targs(scope.allocator);

	if (auto bracketscope = BracketScope(src, itr, kBracketRound))
	{
		while (bracketscope.inner)
		{
			targs.Push(Assume(Traverse<Token::kTypeWord>(src, bracketscope.inner), SyntaxError(src, kErrorExpectedType))->hash);

			if (bracketscope.inner) Assume(Traverse<Token::kTypeSymbol>(src, bracketscope.inner, kSymbolComma), SyntaxError(src));
		}
	}

	auto tmpl = itr;

	Assume(Copy(tmpl), SyntaxError(src));


	auto & ttypes = state.script_ttypes;

	if (Search(ToView({ kobject, kstruct }), tmpl->hash.value))
	{
		Inc(src, itr);

		Traverse<Token::kTypeBracket>(src, itr, kBracketRound);	//skip ParseSpec

		auto name = Assume(Traverse<Token::kTypeWord>(src, itr), SyntaxError(src, kErrorExpectedName));

		auto symbol = ToUInt64(scope.GetNamespace(), name->hash);

		auto & ttype = ttypes.Acquire(symbol) = { &src.token, src.file, tmpl, scope.GetNamespace() };

		ttype.targs = std::move(targs);

		//auto & type = RemoveConst(*ttype.e);

		//type.symbol = Reinterpret<Symbol>(symbol);

		//type.size = sizeof(void *);

		state.RegisterTemplateType(scope.GetNamespace(), AcquireStaticString(state, name->value), ttype.targs.GetSize(), false, {}, InstantiateType);

		Assume(Traverse<Token::kTypeBracket>(src, itr, kBracketBrace), SyntaxError(src, kErrorExpectedBrackets));

		ExpectSemiColon(src, itr);
	}
	else
	{
		if (Traverse<Token::kTypeKeyword>(src, itr, kmethod))
		{
			auto name = Assume(Traverse<Token::kTypeWord>(src, itr), SyntaxError(src, kErrorExpectedTemplateType))->hash;

			auto & ttype = *Assume(ttypes.SearchValue(ToUInt64(scope.GetNamespace(), name)), SyntaxError(src, kErrorExpectedTemplateType));

			ttype.methods.Push(itr);

			while (itr)
			{
				if (itr->type == Token::kTypeBracket && itr->hash == kBracketBrace)
				{
					itr = itr->GetNext();

					return;
				}

				itr = itr->GetNext();
			}

			SyntaxError::Throw(src);
		}
		else
		{
			AnonScope anon(scope);

			Assume(targs.GetSize() < GetArraySize(scope.state.m_targ_placeholders), SyntaxError(src, "unsupported explicit template args"));

			auto pplaceholder = scope.state.m_targ_placeholders;

			REFLEX_FOREACH(targ, targs) anon.RegisterTypedef(targ, *pplaceholder++);


			ScriptTemplateFunction tfn = { src.token, src.file, itr, scope.GetNamespace(), targs };

			Assume(anon.ParseTypeOrAuto(src, itr), SyntaxError(src, kErrorExpectedType));

			auto symbol = Assume(Traverse<Token::kTypeWord>(src, itr), SyntaxError(src, kErrorExpectedName));


			auto fntargs = FunctionHandler::ParseTArgs(src, itr, anon);

			Assume(fntargs.GetSize() == targs.GetSize(), SyntaxError(src, "explicit template args mismatch"));

			pplaceholder = scope.state.m_targ_placeholders;

			for (auto & i : fntargs) Assume(i.type == *pplaceholder++, SyntaxError(src, "explicit template args mismatch"));


			Itr args = Assume(BracketScope(src, itr, kBracketRound), SyntaxError(src, kErrorExpectedBrackets)).inner;

			auto pauto = kAutos;

			while (args)
			{
				//auto start = RemoveConst(args);

				auto & arg = tfn.arguments.Push();

				auto & explicit_arg = tfn.explicit_arguments.Push();

				auto & token = RemoveConst(*args);

				if (TraverseWord(src, args, kauto))
				{
					token.value = *pauto++;
					token.hash = token.value;
				}
				else if (auto type = scope.ParseType(src, args))
				{
					arg.type = type;
				}
				else
				{
					ExpectType(src, args, anon);

					explicit_arg = true;// Search(targs, explicit_t);

					//Inc(src, args);
					//token.value = arg.type->name;
					//token.hash = token.value;
				}

				arg.name = Assume(Traverse<Token::kTypeWord>(src, args), SyntaxError(src))->hash;

				if (args) Assume(Traverse<Token::kTypeSymbol>(src, args, kSymbolComma), SyntaxError(src));
			}

			if (Traverse<Token::kTypeBracket>(src, itr, kBracketBrace))
			{
				state.RegisterTemplateFunction(scope.GetNamespace(), AcquireStaticString(state, symbol->value), targs.GetSize(), tfn.arguments, &InstantiateTemplateFunction::Call, 0);

				state.script_tfns.Insert(ToUInt64(scope.GetNamespace(), symbol->hash), tfn);
			}
			else
			{
				ExpectSemiColon(src, itr);
			}
		}
	}
}

ArrayView <Key32> CompilerImpl::StateImpl::ParseSpec(Source & src, Itr & itr, const ArrayView <Key32> & valid)
{
	spec_cache.Clear();

	Itr inner;

	if (EnterBrackets(src, itr, kBracketRound, inner))
	{
		do
		{
			auto word = Assume(Peek(src, inner, kTypeWord), SyntaxError(src, kErrorExpectedName));

			Inc(src, inner);

			if (!Search(valid, word->hash.value)) ThrowCustomError(src, "invalid type specification '[0]'", { word->value });

			spec_cache.Push(word->hash);
		}
		while (Traverse<Token::kTypeSymbol>(src, inner, kSymbolComma));
	}

	return spec_cache;
}

void CompilerImpl::ParseScope(Source & src, Itr & itr, Scope & scope)
{
	INLINE(void, DispatchIf)(Source & src, Itr & itr, Scope & scope)
	{
		auto & instructions = scope.instructions;

		UInt start = instructions.GetSize();

		auto done = scope.state.AcquireAnonSymbol();

		CompilerImpl::DispatchIf(src, itr, scope, done);

		auto & last = instructions.GetLast();

		if (last.opcode == OPCODE(Marker))
		{
			auto n = instructions.GetSize();

			REFLEX_LOOP_PTR(instructions.GetData() + start, instruction, n - start)
			{
				if (IsJump(instruction->opcode))
				{
					if (instruction->param64 == done.value)
					{
						instruction->param64 = last.param64;
					}
				}
			}
		}
		else
		{
			AddMarker(instructions, src, done);
		}
	}
	END

	INLINE(void, DispatchWhile)(Source & src, Itr & itr, Scope & scope)
	{
		Itr inner = Assume(BracketScope(src, itr, kBracketRound), SyntaxError(src, kErrorExpectedBrackets)).inner;

		AnonScope subscope(scope);

		ExpectConditionalLoopInner<false>(src, subscope, inner, itr);
	}
	END

	INLINE(void, DispatchFor)(Source & src, Itr & itr, Scope & scope)
	{
		Itr inner = Assume(BracketScope(src, itr, kBracketRound), SyntaxError(src, kErrorExpectedBrackets)).inner;

		AnonScope subscope(scope);

		ExpressionHandler::Parse(src, inner, subscope, ExpressionHandler::kContextStatement, scope.bindings.void_t);

		ExpectSemiColon(src, inner);

		ExpectConditionalLoopInner<true>(src, subscope, inner, itr);
	}
	END

	INLINE(void, DispatchTypedef)(Source & src, Itr & itr, Scope & scope)
	{
		auto type = ExpectType(src, itr, scope);

		auto symbol = Expect<Token::kTypeWord>(src, itr)->hash;

		Assume(scope.RegisterTypedef(symbol, type), SyntaxError(src, kErrorDuplicateSymbol));

		ExpectSemiColon(src, itr);
	}
	END

	INLINE(void, DispatchUsing)(Source & src, Itr & itr, Scope & scope)
	{
		auto GetAs = [](Source & src, Itr & itr) -> Pair <Symbol, CString::View>
		{
			auto pair = ParseNamespacedSymbol(src, itr);

			Assume(Copy(pair.b), SyntaxError(src));

			if (TraverseWord(src, itr, kAs))
			{
				auto as = Assume(Peek(src, itr, Token::kTypeWord), SyntaxError(src, kErrorExpectedName))->value;

				Inc(src, itr);

				ExpectSemiColon(src, itr);

				return { { pair.a, pair.b }, as };
			}
			else
			{
				ExpectSemiColon(src, itr);

				return { { pair.a, pair.b }, pair.b };
			}
		};

		Key32 type = knamespace;

		if (auto bracketscope = BracketScope(src, itr, kBracketRound)) type = Assume(Copy(bracketscope.inner), SyntaxError(src))->hash;

		switch (type.value)
		{
		case knamespace:
		{
			scope.RegisterUsing(scope.ResolveNamespace(src, Assume(ParseNamespaceChain(src, itr), SyntaxError(src, kErrorExpectedNamespace))));
			ExpectSemiColon(src, itr);
		}
			break;

		case K32("type"):
		{
			auto type = ExpectType(src, itr, scope);

			auto token = type->name;

			if (TraverseWord(src, itr, K32("as")))
			{
				token = Assume(Peek(src, itr, Token::kTypeWord), SyntaxError(src, kErrorExpectedName))->value;

				Inc(src, itr);
			}

			Assume(scope.RegisterTypedef(token, type), SyntaxError(src, kErrorDuplicateSymbol));

			ExpectSemiColon(src, itr);
		}
		break;

		case K32("function"):
		{
			auto symbol = GetAs(src, itr);

			auto functions = Assume(scope.bindings.GetExternalFunctions(symbol.a), SyntaxError(src, kErrorTemplateUnknownSymbol));

			for (auto & i : functions)
			{
				auto & fn = i.value;

				scope.bindings.RegisterFunction(scope.GetNamespace(), symbol.b, fn.rtn, fn.args, fn.targs, fn.clientdata, fn.flags2, fn.externalfnptr);
			}
		}
		break;

		case K32("define"):
		{
			auto symbol = GetAs(src, itr);

			auto constv = Assume(scope.state.GetConstValue(symbol.a), SyntaxError(src, kErrorTemplateUnknownSymbol));

			Data::Archive value = Data::Pack(constv->value);

			value.SetSize(constv->type->size);

			scope.state.RegisterConstant(constv->type, scope.GetNamespace(), symbol.b, value);// .documentable = false;
		}
		break;

		default:
			SyntaxError::Throw(src);
			break;
		}
	}
	END

	INLINE(void, DispatchMethodDeclaration)(Source & src, Itr & itr, Scope & scope)
	{
		auto type = ExpectType(src, itr, scope);

		auto nss = MakeNamespacedSymbol(scope.state, scope.GetNamespace(), type->name);

		NamespaceScope nsscope(scope, nss);

		Assume(type->IsObject(), SyntaxError(src, kErrorStructsCanNotHaveMethods));

		if (!FunctionHandler::ParseMethod(src, itr, nsscope, type))
		{
			Assume(TraverseWord(src, itr, type->symbol.b), SyntaxError(src, kErrorExpectedMemberOrMethod));

			FunctionHandler::AssumeConstructor(src, itr, nsscope, type);
		}
	}
	END

	INLINE(void,DispatchNamespace)(Source & src, Itr & itr, Scope & scope)
	{
		Assume(scope.IsGlobal(), SyntaxError(src, kErrorExpectedNamespace));

		auto & symbol = Assume(Traverse<Token::kTypeWord>(src, itr), SyntaxError(src, kErrorExpectedName))->value;

		Itr inner = Assume(BracketScope(src, itr, kBracketBrace), SyntaxError(src)).inner;

		NamespaceScope subscope(scope, symbol);

		ParseScope(src, inner, subscope);
	}
	END

	auto Traverse = [](Source & src, Itr & itr) -> Itr &
	{
		src.line = UInt16(itr->linenumber);

		itr = itr->GetNext();

		return itr;
	};

	while (itr)
	{
		auto token_type = itr->type;

		auto token = itr->hash.value;

		if (token_type == Token::kTypeKeyword)
		{
			switch (token)
			{
			case kif:
				DispatchIf::Call(src, Traverse(src, itr), scope);
				break;

			case kforeach:
				DispatchForeach(src, Traverse(src, itr), scope);
				break;

			case kfor:
				DispatchFor::Call(src, Traverse(src, itr), scope);
				break;

			case kwhile:
				DispatchWhile::Call(src, Traverse(src, itr), scope);
				break;

			case kswitch:
				DispatchSwitch(src, Traverse(src, itr), scope);
				break;

			case kreturn:
			case kbreak:
			case kcontinue:
			case kexit:
				scope.DispatchExitKeyword(Inc(src, itr).hash, src, itr);
				break;

			case kobject:
				Inc(src, itr);
				DispatchClass(src, itr, scope, scope.GetNamespace(), true);
				break;

			case kstruct:
				Inc(src, itr);
				DispatchClass(src, itr, scope, scope.GetNamespace(), false);
				break;

			case ktypedef:
				DispatchTypedef::Call(src, Traverse(src, itr), scope);
				break;

			case kusing:
				DispatchUsing::Call(src, Traverse(src, itr), scope);
				break;

			case ktemplate:
				DispatchTemplate(src, Traverse(src, itr), scope);
				break;

			case kmethod:
				DispatchMethodDeclaration::Call(src, Traverse(src, itr), scope);
				break;

			case knamespace:
				DispatchNamespace::Call(src, Traverse(src, itr), scope);
				break;

			case kstatic:
				{
					Assume(!scope.IsGlobal(), SyntaxError(src));

					Inc(src, itr);

					struct StaticScope : public Scope
					{
						StaticScope(Scope & global, Scope & current)
							: Scope(current.state, true, true, &current, global.stackframelayout, global.variables, global.instructions, current.GetNamespace())
						{
						}
					}

					//staticscope(*scope.state.scopes.GetFirst(), scope);
					staticscope(*scope.state.root, scope);

					ExpectStatement(src, itr, staticscope);
				}
				break;

			default:
				ExpectStatement(src, itr, scope);
				break;
			}
		}
		else if (token_type == Token::kTypeSymbol && token == kSymbolHash)
		{
			Assume(scope.IsGlobal(), SyntaxError(src));

			DispatchPreprocessor(src, Traverse(src, itr), scope);
		}
		else if (!FunctionHandler::ParseFunctionDeclaration(src, itr, scope, scope.GetNamespace()))		//
		{
			ExpectScopedStatement<AnonScope>(src, itr, scope);
		}
	}
}

TypeRef CompilerImpl::DispatchClass(Source & src, Itr & itr, Scope & scope, Key32 ns, bool OBJECT, CString::View name)
{
	typedef LayoutTemplateImpl <true> LayoutTemplateX;

	INLINE(UInt16, AddCommon)(Scope & scope, Source & src, Itr & itr, bool isconst, TypeRef type, LayoutTemplateX & layout, Variables & members)
	{
		auto symbol = Expect<Token::kTypeWord>(src, itr)->hash;

		for (auto & i : members) Assume(i.a != symbol, SyntaxError(src, kErrorDuplicateSymbol));

		auto address = UInt16(layout.init_state.GetSize());

		members.Push({ symbol, MakeMember(type, address, isconst) });

		return address;
	}
	END

	INLINE(void, AddObject)(Scope & scope, Source & src, Itr & itr, bool isconst, TypeRef type, LayoutTemplateX & layout, Variables & members)
	{
		auto address = AddCommon::Call(scope, src, itr, isconst, type, layout, members);

		auto ctr = type->ctr;

		if (Traverse<Token::kTypeSymbol>(src, itr, kSymbolEqual))
		{
			if (Traverse<Token::kTypeKeyword>(src, itr, knew))
			{
				Assume(IsDefaultConstructable(type), SyntaxError(src, kErrorMatchingConstructorNotFound));
			}
			else if (Traverse<Token::kTypeKeyword>(src, itr, knull))
			{
				Assume(IsExplicitNullable(scope.bindings, type), SyntaxError(src, kErrorNonNullableType));

				ctr = type->null;
			}
			else
			{
				SyntaxError::Throw(src, kErrorExpectedNewOrNull);
			}
		}
		else
		{
			Assume(IsDefaultConstructable(type), SyntaxError(src, kErrorMatchingConstructorNotFound));
		}

		layout.objects.Push({ UInt16(address), type, ctr });

		MemCopy(&REFLEX_NULL(Object), Extend(layout.init_state, sizeof(void *)).data, sizeof(void *));
	}
	END

	INLINE(void, AddValue)(Scope & scope, Source & src, Itr & itr, bool isconst, TypeRef type, LayoutTemplateX & layout, Variables & members)
	{
		AddCommon::Call(scope, src, itr, isconst, type, layout, members);

		if (Traverse<Token::kTypeSymbol>(src, itr, kSymbolEqual))
		{
			auto value = ExpectConstLiteral(src, scope, itr);

			Assume(value.a == type, SyntaxError(src, kErrorTypeMismatch));

			layout.init_state.Append(value.b);
		}
		else
		{
			layout.init_state.Append(type->params);
		}
	}
	END

	INLINE(void,AssumeSkipBody)(Source & src, Itr & itr)
	{
		Assume(itr && Or(itr->hash == kBracketBrace, itr->hash == kSymbolSemiColon), SyntaxError(src));

		Inc(src, itr);
	}
	END


	//Inc(src, itr);

	auto & state = scope.state;

	auto & bindings = scope.bindings;

	bool mt = True(Search(state.ParseSpec(src, itr, kTypeSpecs[OBJECT]), K32("mt")));

	auto & nametoken = Inc(src, itr);

	name = name ? name : nametoken.value;


	Symbol symbol = { ns, nametoken.hash };

	Assume(!GetTypeBySymbol(bindings, symbol), SyntaxError(src, kErrorDuplicateSymbol));

	Itr inner = Assume(BracketScope(src, itr, kBracketBrace), SyntaxError(src)).inner;

	ExpectSemiColon(src, itr);


	auto nss = MakeNamespacedSymbol(state, ns, name);

	NamespaceScope nsscope(scope, nss);

	//auto & nsscope = scope;


	Variables members;

	LayoutTemplateX layout;

	Array <Itr> constructors(scope.allocator);

	Array <Itr> methods(scope.allocator);

	while (inner)
	{
		auto store = inner;

		if (Traverse<Token::kTypeKeyword>(src, inner, kobject))
		{
			DispatchClass(src, inner, nsscope, nsscope.GetNamespace(), true);
		}
		else if (Traverse<Token::kTypeKeyword>(src, inner, kstruct))
		{
			DispatchClass(src, inner, nsscope, nsscope.GetNamespace(), false);
		}
		else if (Traverse<Token::kTypeKeyword>(src, inner, kstatic))
		{
			if (!FunctionHandler::ParseFunctionDeclaration(src, inner, nsscope, nsscope.GetNamespace()))		//
			{
				ExpectStatement(src, inner, nsscope);
			}
		}
		else if (Traverse<Token::kTypeKeyword>(src, inner, ktemplate))
		{
			Assume(false, SyntaxError(src));
		}
		else if (TraverseWord(src, inner, nametoken.hash))
		{
			Assume(Copy(OBJECT), SyntaxError(src, kErrorStructsCanNotHaveMethods));

			auto itr = inner;

			if (Traverse<Token::kTypeBracket>(src, inner, kBracketRound))
			{
				constructors.Push(itr);
			}
			else if (auto name = Traverse<Token::kTypeWord>(src, inner))
			{
				methods.Push(itr->GetPrev());

				Assume(Traverse<Token::kTypeBracket>(src, inner, kBracketRound), SyntaxError(src, kErrorExpectedBrackets));
			}
			else
			{
				Assume(false, SyntaxError(src));
			}

			AssumeSkipBody::Call(src, inner);
		}
		else
		{
			auto type = ExpectType(src, inner, nsscope);

			auto isconst = false;

			auto name = inner;

			Assume(Traverse<Token::kTypeWord>(src, inner), SyntaxError(src, kErrorExpectedName));

			if (Traverse<Token::kTypeBracket>(src, inner, kBracketRound))
			{
				Assume(Copy(OBJECT), SyntaxError(src, kErrorStructsCanNotHaveMethods));

				AssumeSkipBody::Call(src, inner);

				methods.Push(store);
			}
			else
			{
				inner = name;

				if (type->IsObject())
				{
					Assume(Copy(OBJECT), SyntaxError(src, kErrorStructsCanNotContainObjects));

					if (mt && !type->flags.Check(Type::kFlagThreadsafe)) state.ThrowCustomError(src, kErrorMemberIsNotMtCompatible, { type->name });

					AddObject::Call(nsscope, src, inner, isconst, type, layout, members);
				}
				else
				{
					AddValue::Call(nsscope, src, inner, isconst, type, layout, members);
				}

				while (Traverse<Token::kTypeSymbol>(src, inner, kSymbolComma))
				{
					if (type->IsObject())
					{
						AddObject::Call(nsscope, src, inner, isconst, type, layout, members);
					}
					else
					{
						AddValue::Call(nsscope, src, inner, isconst, type, layout, members);
					}
				}

				ExpectSemiColon(src, inner);
			}
		}
		//else
		//{
		//	Assume(false, SyntaxError(src));
		//}
	}

	auto staticname = AcquireStaticString(state, name);

	auto & currentpath = state.currentpath;

	auto filename = Nudge(currentpath.b, -Reinterpret<Int>(currentpath.a.size));

	if (OBJECT)
	{
		auto type = RemoveConst(ScriptObject::RegisterType(state, filename, ns, staticname, std::move(members), &layout, mt));

		auto defaultctr = !constructors;

		for (auto & i : constructors)
		{
			defaultctr = defaultctr || !FunctionHandler::AssumeConstructor(src, i, nsscope, type).args;
		}

		for (auto & i : methods)
		{
			Assume(FunctionHandler::ParseMethod(src, i, nsscope, type), SyntaxError(src, kErrorExpectedMemberOrMethod));
		}

		type->flags.Set(Type::kFlagDefaultConstructable, defaultctr);

		return type;
	}
	else
	{
		UInt size = layout.init_state.GetSize();

		//auto stride = RoundUpPow2(size);

		layout.init_state.SetSize(size);

		auto type = Assume(CreateValueType(bindings, GenerateScriptTypeID(MakeNamespacedSymbol(state, ns, staticname), filename), ns, staticname, layout.init_state), SyntaxError(src, kErrorInvalidSize));

		type->members = std::move(members);

		type->size = UInt8(size);

		return type;
	}
}

TypeRef CompilerImpl::ExpectType(Source & src, Itr & itr, Scope & scope)
{
	return Assume(scope.ParseType(src, itr), SyntaxError(src, kErrorExpectedType));
}

void CompilerImpl::ExpectStatement(Source & src, Itr & itr, Scope & scope)
{
	auto type = Assume(ExpressionHandler::Parse(src, itr, scope, ExpressionHandler::kContextStatement, scope.bindings.void_t), SyntaxError(src, kErrorExpectedStatement));

	AddDiscardVariable(scope.instructions, src, type);

	ExpectSemiColon(src, itr);
}

template <class SUBSCOPE> void CompilerImpl::ExpectScopedStatement(Source & src, Itr & itr, Scope & scope)
{
	if (auto brackets = BracketScope(src, itr, kBracketBrace))
	{
		Itr inner = brackets.inner;

		if (inner)
		{
			SUBSCOPE subscope(scope);

			ParseScope(src, inner, subscope);
		}
	}
	else
	{
		ExpectStatement(src, itr, scope);
	}
}

Variable CompilerImpl::ExpectConditionalExpression(Source & src, Itr & itr, Scope & scope, UInt8 context)
{
	//TODO add bool type
	//TODO add cast to bool op: bool operatorCast(TYPE) as compiler intrinsic
	// then this always works on bool
	// optimiser will convert int/float -> bool -> jumpiftrue to int/float -> jumpiftrue32
	// but need true bool type, (int32 -> uint8 is different to int32 -> bool)
	//
	//look this up in autocast
	//this is then simply ExpressionHandler::Parse(src, scope, itr, bindings.bool_t, false)
	//optimizer:
	//if And(opcode[0] == CallExternal Value32ToBool, opcode[+1] == JumpIfTrue8 or JumpIfFalse8) replace with JumpIfTrue32 or JumpIfFalse32

	//RestorePoint restore(scope, itr);

	auto & bindings = scope.bindings;

	Assume(Copy(itr), SyntaxError(src, kErrorExpectedBooleanExpression));

	auto variable = Assume(ExpressionHandler::ParseX(src, itr, scope, ExpressionHandler::Context(context), bindings.bool_t), SyntaxError(src, kErrorExpectedBooleanExpression));

	if (variable.type == bindings.bool_t)
	{
		return variable;
	}
	else if (!variable.type->IsObject())
	{
		//TODO clean solution, generic opCast@bool operator on all values, which is optimised out in executable for bool/uint8 and int/float
		//this currently does not work for if ('a' && 'b') because no bool casting, and opAnd only implemented for bool

		ArrayView <TypeRef> integrals = { &bindings.bool_t, 4 };

		REFLEX_ASSERT(integrals.data + 3 == &bindings.float32_t);

		for (auto & i : integrals)
		{
			if (i == variable.type) return variable;

		}

		AddPushConst(scope.instructions, src, variable.type->params);

		AddComparisonIntrinsic(scope.instructions, src, variable.type, OPCODE(intrinsicValueInequal));

		return { variable.type };
	}

	SyntaxError::Throw(src, kErrorExpectedBooleanExpression);

	return {};
}

void CompilerImpl::DispatchIf(Source & src, Itr & itr, Scope & scope, Key32 donemarker)
{
	auto & state = scope.state;

	auto & instructions = scope.instructions;

	Itr child = Assume(BracketScope(src, itr, kBracketRound), SyntaxError(src, kErrorExpectedBrackets)).inner;

	auto elsemarker = state.AcquireAnonSymbol();


	AnonScope ifscope(scope);

	auto var = ExpectConditionalExpression(src, child, ifscope, ExpressionHandler::kContextConditional);

	AddConditionalJump(instructions, src, elsemarker, var.type->size, OPCODE(JumpIfFalse8), OPCODE(JumpIfFalse32));

	ExpectScopedStatement<DummyScope>(src, itr, ifscope);

	AddJump(instructions, src, donemarker);

	AddMarker(instructions, src, elsemarker);

	if (Traverse<Token::kTypeKeyword>(src, itr, kelse))
	{
		if (Traverse<Token::kTypeKeyword>(src, itr, kif))
		{
			DispatchIf(src, itr, scope, donemarker);
		}
		else
		{
			ExpectScopedStatement<DummyScope>(src, itr, ifscope);
		}
	}
}

REFLEX_INLINE void CompilerImpl::DispatchSwitch(Source & src, Itr & itr, Scope & scope)
{
	auto ParseCase = [](Source & src, Itr & caseitr, Scope & scope, TypeRef type, Array <UInt32> & values)
	{
		auto value = ExpectConstLiteral(src, scope, caseitr).b;

		switch (value.size)
		{
		case 1:
			values.Push(value[0]);
			break;

		case 4:
			values.Push(Reinterpret<UInt32>(Data::Unpack<Int32>(value)));
			break;
		}
	};

	INLINE(void, ParseCaseStatement)(Source & src, Itr & itr, Scope & scope, Instructions & instructions, SwitchTable & table, UInt32 value)
	{
		auto & state = scope.state;

		auto markerid = AddMarker(instructions, src, state.AcquireAnonSymbol());

		table.cases.Push({ value, UIntNative(markerid.value) });

		Assume(True(itr), SyntaxError(src));

		ExpectScopedStatement<AnonScope>(src, itr, scope);
	}
	END

	LOCAL(void, ParseCaseContent)(Source & src, Itr & itr, Scope & scope, Instructions & instructions, SwitchTable & table, UInt32 value)
	{
		struct SwitchScope : public SubScope
		{
			SwitchScope(Scope & parent)
				: SubScope(parent)
			{
			}

			virtual bool OnHandleExit(Key32 keyword, Source & src, Itr & itr) override
			{
				switch (keyword.value)
				{
				case kreturn:
					Cast<FunctionHandler::Body>(GetRoot())->DispatchReturn(src, itr, *this);
					ExpectSemiColon(src, itr);
					throw(this);
					return true;

				case kbreak:
					ExpectSemiColon(src, itr);
					throw(this);
					return true;

				default:
					state.ThrowCustomError(src, kErrorTemplateInvalidKeyword, { GetExitKeyword(keyword) });
					return false;
				}
			}
		};

		auto & state = scope.state;

		auto markerid = AddMarker(instructions, src, state.AcquireAnonSymbol());

		table.cases.Push({ value, UIntNative(markerid.value) });

		SwitchScope subscope(scope);

		try
		{
			ParseScope(src, itr, subscope);
		}
		catch (SwitchScope *)
		{
			return;
		}

		Assume(false, SyntaxError(src, kErrorExpectedBreak));
	}
	END

	auto & state = scope.state;

	auto & bindings = scope.bindings;

	auto & instructions = scope.instructions;

	auto int32_t = bindings.int32_t;

	auto uint8_t = bindings.uint8_t;

	if (auto bracketscope = BracketScope(src, itr, kBracketRound))
	{
		auto expr = bracketscope.inner;

		auto exit = state.AcquireAnonSymbol();

		//TODO when seperate bool tyoe, set the target type as Uint8
		//then it will be default type
		//currently doesnt work as any value can cast to bool
		
		auto type = ExpressionHandler::Parse(src, expr, scope, ExpressionHandler::kContextTemporary, 0);

		//auto type = ExpressionHandler::Parse(src, expr, scope, ExpressionHandler::kContextTemporary, uint8_t);

		auto table = Cast<SwitchTable>(Cast<ProgramImpl>(state.m_target)->data.Push(REFLEX_CREATE(SwitchTable)));

		table->pinstructions = &scope.instructions;

//		if (type == bindings.string_t)
//		{
//			type = bindings.key32_t;
//		}
//		else 
		if (type == uint8_t || type == int32_t || type == bindings.key32_t)
		{
		}
		else
		{
			SyntaxError::Throw(src, kErrorExpectedConstant);
		}

		AddSwitch(instructions, src, type->size == 4, table);

		auto handler = &ParseCaseContent::Call;

		if (auto casebracketscope = EnterBrackets(src, itr, kBracketBrace))
		{
			Itr caseitr = casebracketscope.inner;

			Array <UInt32> values(scope.allocator);

			while (Peek(src, caseitr, kcase))//SWAP
			{
				values.Clear();

				while (Traverse<Token::kTypeKeyword>(src, caseitr, kcase))
				{
					if (auto innerbrackets = EnterBrackets(src, caseitr, kBracketRound))
					{
						ParseCase(src, innerbrackets.inner, scope, type, values);

						handler = &ParseCaseStatement::Call;

						break;
					}
					else
					{
						ParseCase(src, caseitr, scope, type, values);

						Expect<Token::kTypeSymbol>(src, caseitr, kSymbolColon);

						handler = &ParseCaseContent::Call;
					}
				}

				auto copy = caseitr;

				for (auto & i : values)
				{
					caseitr = copy;

					handler(src, caseitr, scope, instructions, table, i);

					AddJump(instructions, src, exit);
				}
			}

			if (Traverse<Token::kTypeKeyword>(src, caseitr, kDefault))
			{
				Expect<Token::kTypeSymbol>(src, caseitr, kSymbolColon);

				handler(src, caseitr, scope, instructions, table, kMaxUInt32);
			}
			else
			{
				table->cases.Push({ kMaxUInt32, UIntNative(exit.value) });
			}

			AddMarker(instructions, src, exit);

			Assume(!caseitr, SyntaxError(src));

			return;
		}
	}

	SyntaxError::Throw(src);
}

REFLEX_NOINLINE CString::View CompilerImpl::ParseNamespaceChain(Source & src, Itr & itr)
{
	LOCAL(void, Next)(Source & src, Itr & itr, CString::View & rtn)
	{
		Assume(Assume(Copy(itr), SyntaxError(src))->type == Token::kTypeWord, SyntaxError(src));

		CString::View word = Inc(src, itr).value;

		if (itr)
		{
			if (itr->hash == kSymbolNamespaceDelimiter)
			{
				Inc(src, itr);

				Call(src, itr, rtn);

				return;
			}
		}

		rtn = word;
	}
	END

	if (itr ? (itr->type == Token::kTypeWord) : false)
	{
		auto begin = itr->value;

		auto end = begin;

		Next::Call(src, itr, end);

		return { begin.data, UInt((end.data + end.size) - begin.data) };
	}
	else
	{
		return {};
	}
}

REFLEX_NOINLINE Pair <CString::View> CompilerImpl::SplitNamespacedSymbol(const CString::View & value)
{
	if (auto pos = ReverseSearch(value, kNamespaceDelimiter))
	{
		auto [ns, symbol] = Splice(value, pos.value);

		symbol = Nudge(symbol, kNamespaceDelimiter.size);

		return { ns, symbol };
	}
	else
	{
		return { {}, value };
	}
}

Pair <TypeRef,Data::Archive::View> CompilerImpl::ExpectConstLiteral(Source & src, Scope & scope, Itr & itr)
{
	auto var = ExpressionHandler::ParseX(src, itr, scope, ExpressionHandler::kContextTemporary, 0);

	Assume(var.location == Location::kConst, SyntaxError(src, kErrorExpectedConstant));

	auto & state = scope.state;

	auto & instructions = scope.instructions;

	auto type = var.type;

	auto value = instructions.GetLast().param64;

	instructions.Pop();

	state.constant_cache = Data::Archive::View(Reinterpret<UInt8>(&value), type->size);

	return { type, state.constant_cache };
}

Tuple < Array<TypeRef>, Array<Key32> > & CompilerImpl::StateImpl::RetrieveInheritanceInfo(TypeRef type)
{
	auto & cache = inheritancechains_cache;

	auto & usings = cache.Acquire(type);

	if (!usings.b)
	{
		if (auto objectclass = type->object_t)
		{
			Key32 last = kZeroKey;

			while (objectclass)
			{
				if (auto type = bindings->GetTypeByRTTID(objectclass->type_id))
				{
					if (SetFiltered(last, type->symbol.a))
					{
						usings.b.Insert(0, last);
					}

					usings.a.Push(type);
				}

				objectclass = objectclass->base;
			}
		}
		else
		{
			usings.a.Push(type);

			usings.b.Push(type->symbol.a);
		}
	}

	return usings;
}

bool CompilerImpl::StateImpl::InheritsFrom(TypeRef type, TypeRef base)
{
	auto & chain = RetrieveInheritanceInfo(type).a;

	return True(Search(chain, base));
}

CompilerImpl::InstructionsRestorePoint::InstructionsRestorePoint(Scope & scope)
	: scope(scope),
	m_ninstruction(scope.instructions.GetSize()),
	m_layout_size(scope.stackframelayout.init_state.GetSize()),
	m_layout_nobject(UInt16(scope.stackframelayout.objects.GetSize())),
	m_layout_nvarnames(UInt16(scope.stackframelayout.varnames.GetSize())),
	m_nvariables(UInt16(scope.variables.GetSize())),
	m_nusings(UInt16(scope.m_usings.GetSize()))
{
}

void CompilerImpl::InstructionsRestorePoint::Rollback()
{
	scope.instructions.SetSize(m_ninstruction);

	auto & layout = scope.stackframelayout;

	layout.init_state.SetSize(m_layout_size);

	layout.objects.SetSize(m_layout_nobject);

	layout.varnames.SetSize(m_layout_nvarnames);

	scope.variables.SetSize(m_nvariables);

	scope.m_usings.SetSize(m_nusings);
}

CompilerImpl::RestorePoint::RestorePoint(Scope & scope, Itr & itr)
	: ParserRestorePoint(itr),
	InstructionsRestorePoint(scope)
{
}

void CompilerImpl::RestorePoint::Rollback()
{
	ParserRestorePoint::Rollback();

	InstructionsRestorePoint::Rollback();
}

bool CompilerImpl::ExternalConstsAccessor::Get(Scope & scope, Symbol symbol, const Param & flags, Output & map)
{
	if (auto c = scope.state.GetConstValue(symbol))
	{
		map.Push(c);
	}

	return true;
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::VM::Compiler> Reflex::VM::Compiler::Create()
{
	return REFLEX_CREATE(Detail::CompilerImpl);
}

void Reflex::VM::Detail::PublishError(File::ResourcePool & resourcepool, const WString & path, UInt line, CString && stage, CString && msg)
{
	output.Error(stage, line, msg);

	File::ResourcePool::Lock lock(resourcepool);

	if (auto token = File::Query(lock, REFLEX_TYPEID(Data::ArchiveObject), path))
	{
		Data::SetError(RemoveConst(token)->object, line, std::move(stage), std::move(msg));
	}
	else
	{
		REFLEX_ASSERT(false);
	}
}

Reflex::VM::Compiler::State::State(Bindings & bindings)
	: bindings(bindings)
{
	Retain(bindings);
}

Reflex::VM::Compiler::State::~State()
{
	Release(bindings);
}

Reflex::CString Reflex::VM::Detail::MakeTemplateName(Compiler::State & tbindings, CString::View name, const ArrayView <TypeRef> & targs)
{
	auto AddNamespace = [](Compiler::State & state, Key32 ns, CString & out)
	{
		if (auto string = GetNamespaceString(state, ns))
		{
			out.Append(Join(string, kNamespaceDelimiter));
		}
	};

	CString tname;

	tname.Allocate(16);

	tname.Append(name);

	tname.Push('@');

	if (targs.size == 1)
	{
		auto & type = *targs[0];

		AddNamespace(tbindings, type.symbol.a, tname);

		tname.Append(type.name);
	}
	else
	{
		tname.Push('(');

		for (auto & i : targs)
		{
			auto & type = *i;

			AddNamespace(tbindings, type.symbol.a, tname);

			tname.Append(type.name);

			tname.Push(',');
		}

		tname.Pop();

		tname.Push(')');
	}

	return tname;
}

Reflex::TRef <Reflex::Object> Reflex::VM::Detail::OpenSourceFile(const File::ResourcePool::StreamContext & ctx, System::FileHandle & instream)
{
	return REFLEX_CREATE(CompilerImpl::SourceFileImpl, File::ReadBytes(instream));
}

void Reflex::VM::Detail::AddSource(Program & program, String & path, TypeID type_id, Object & source)
{
	RemoveConst(program.sources).Push({ path, path.GetView(), { path.hash, type_id }, source });
}

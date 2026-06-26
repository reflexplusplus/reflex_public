#pragma once

#include "tokeniser.h"
#include "error.h"




//
//

#define IS_RETAINED(var) REFLEX_ASSERT(var.location == Location::kTemporary ? !var.type->IsObject() : true)

#define CONSTANT(symbol, def) static inline const UInt32 symbol = K32(def);

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

struct CompilerImpl : public Compiler
{
	struct CloneableState;

	struct StateImpl;

	struct SourceFileImpl;

	CONSTANT(knamespace, "namespace");
	CONSTANT(ktemplate, "template");
	CONSTANT(kstruct, "struct");
	CONSTANT(kobject, "object");
	CONSTANT(kmethod, "method");
	CONSTANT(kstatic, "static");
	CONSTANT(kconst, "const");
	CONSTANT(kauto, "auto");
	CONSTANT(kfor, "for");
	CONSTANT(kwhile, "while");
	CONSTANT(kforeach, "foreach");
	CONSTANT(kif, "if");
	CONSTANT(kelse, "else");
	CONSTANT(kswitch, "switch");
	CONSTANT(kcase, "case");
	CONSTANT(kDefault, "default");
	CONSTANT(kbreak, "break");
	CONSTANT(kcontinue, "continue");
	CONSTANT(kreturn, "return");
	CONSTANT(kexit, "exit");
	CONSTANT(knull, "null");
	CONSTANT(knew, "new");
	CONSTANT(kbind, "bind");

	CONSTANT(kusing, "using");
	CONSTANT(kAs, "as");
	CONSTANT(ktypedef, "typedef");
	CONSTANT(ktypeid, "typeid");
	CONSTANT(ktypeof, "typeof");
	CONSTANT(ksizeof, "sizeof");
	CONSTANT(kreinterpret, "reinterpret");

	CONSTANT(kPublic, "public");
	CONSTANT(kProtected, "protected");
	CONSTANT(kPrivate, "private");

	CONSTANT(kInline, "inline");

	CONSTANT(kSymbolHash, "#");
	CONSTANT(kSymbolNamespaceDelimiter, "::");
	CONSTANT(kSymbolAt, "@");
	CONSTANT(kSymbolEqual, "=");
	CONSTANT(kSymbolSemiColon, ";");
	CONSTANT(kSymbolComma, ",");
	CONSTANT(kSymbolColon, ":");
	CONSTANT(kSymbolDot, ".");

	CONSTANT(kBracketRound, "(");
	CONSTANT(kBracketSquare, "[");
	CONSTANT(kBracketBrace, "{");


	struct ParserRestorePoint;

	struct InstructionsRestorePoint;

	struct RestorePoint;


	struct ExternalConstsAccessor;

	struct ExternalObjectAccessor;

	typedef const Token * Itr;


	class Scope;

	struct SubScope;

	struct AnonScope;

	struct NamespaceScope;

	struct DummyScope
	{
		DummyScope(Scope & parent)
			: parent(parent)
		{
		}

		operator Scope & () { return parent; }

		Scope & parent;
	};


	struct FunctionHandler;

	struct CallHandler;

	struct ExpressionHandler;

	struct Source : public VM::Source
	{
		Source(const Token & token, UInt8 file, UInt16 line)
			: VM::Source({ line, file }),
			token(token)
		{
		}

		const Token & token;
	};


	virtual TRef <State> CreateState(UInt8 contextflags) const override;

	virtual TRef <Program> Compile(File::ResourcePool::Lock & lock, const WString::View & source, UInt8 contextflags, Object & clientdata, const ArrayView <ConstTRef<Module>> & defaultmodules, const State & prebinding) const override;



	//internal

	static const Token & Inc(Source & src, Itr & itr);

	template <Token::Type TYPE> static Itr Traverse(Source & src, Itr & itr, Key32 value = kNullKey);

	static Itr Peek(Source & src, const Itr & itr, Key32 value)
	{
		if (itr) if (itr->hash == value) return itr;

		return 0;
	}

	static Itr Peek(Source & src, const Itr & itr, Token::Type type)
	{
		if (itr) if (itr->type == type) return itr;

		return 0;
	}


	static Itr TraverseWord(Source & src, Itr & itr, Key32 value);


	struct BracketScope	//this is to get a more correct error reporting of line
	{
		BracketScope(Source & src, Itr & token, Key32 value);

		~BracketScope();

		operator bool() const { return m_token; }

		Itr inner;



	private:

		Source & src;

		Itr m_token;

		UInt16 line;
	};

	static BracketScope EnterBrackets(Source & src, Itr & itr, Key32 value, Itr & rtn);

	static BracketScope EnterBrackets(Source & src, Itr & itr, Key32 value) { return BracketScope(src, itr, value); }


	template <Token::Type TYPE> static Itr Expect(Source & src, Itr & itr, Key32 hash = kNullKey);

	static void ExpectSemiColon(Source & src, Itr & itr);


	static CString::View ParseNamespaceChain(Source & src, Itr & itr);

	static Pair <CString::View> ParseNamespacedSymbol(Source & src, Itr & itr);

	static Pair <CString::View> SplitNamespacedSymbol(const CString::View & symbol);


	static TypeRef ExpectType(Source & src, Itr & itr, Scope & scope);

	static void ExpectStatement(Source & src, Itr & itr, Scope & scope);	//for case with nothing on LHS

	template <class SUBSCOPE> static void ExpectScopedStatement(Source & src, Itr & itr, Scope & scope);	//for case with nothing on LHS

	static Variable ExpectConditionalExpression(Source & src, Itr & itr, Scope & scope, UInt8 context);

	template <bool POSTOP> static void ExpectConditionalLoopInner(Source & src, AnonScope & outer, Itr & test, Itr & itr);


	static void DispatchPreprocessor(Source & src, Itr & itr, Scope & global);

	static TypeRef DispatchClass(Source & src, Itr & itr, Scope & scope, Key32 ns, bool object, CString::View name = {});

	static void DispatchForeach(Source & src, Itr & itr, Scope & scope);

	static void DispatchIf(Source & src, Itr & itr, Scope & scope, Key32 done_marker);

	static void DispatchSwitch(Source & src, Itr & itr, Scope & scope);


	static Pair <TypeRef, Data::Archive::View> ExpectConstLiteral(Source & src, Scope & scope, Itr & itr);


	static void DispatchTemplate(Source & src, Itr & itr, Scope & scope);



	//helpers

	template <class ACCESSOR> static auto GetSymbolsOfType(Scope & scope, const CString::View & ns, typename ACCESSOR::SymbolType symbol, const ArrayView <Key32> & usings, const typename ACCESSOR::Param & param = {});

	static void ParseScope(Source & src, Itr & itr, Scope & scope);

	static CString::View GetExitKeyword(Key32 keyword)
	{
		switch (keyword.value)
		{
		case kbreak:
			return "break";

		case kcontinue:
			return "continue";

		case kreturn:
			return "return";

		case kexit:
			return "exit";
		}

		return "";
	}

	struct ScriptTemplate
	{
		ConstReference <Token> root;

		UInt8 file;

		Itr decl;

		Key32 ns;
	};

	struct ScriptTemplateType : public ScriptTemplate
	{
		Array <Key32> targs;
		Array <Itr> methods;
	};

	struct ScriptTemplateFunction : public ScriptTemplate
	{
		Array <Key32> targs;
		Array <Argument> arguments;	//0 for auto
		Array <Idx> explicit_arguments;
	};

	typedef ObjectOf < Sequence < Key64, Reference <String> > > StringPool;



	const Reference <File::ResourcePool> m_resourcepool;


	static inline const Key32 kDelimiters[2] = { kSymbolNamespaceDelimiter, kSymbolAt };

	static inline const ArrayView <Key32> kNamespaced = { kDelimiters, 1 };

	static inline const ArrayView <Key32> kTemplated = { kDelimiters, 2 };


	static inline const Key32 kObjectSpecs[] = { K32("mt") };

	static inline const Key32 kStructSpecs[] = { kNullKey };

	static inline const ArrayView <Key32> kTypeSpecs[] = { ToView(kStructSpecs), ToView(kObjectSpecs) };

};

struct CompilerImpl::CloneableState : public Compiler::State
{
	using Compiler::State::State;

	virtual TRef <StateImpl> Clone(UInt8 contextflags, Object & client) const = 0;	//this is for optimisation -> pre-bind then clone, rather than build from scratch each compile
};

struct CompilerImpl::StateImpl : public CloneableState
{
	typedef Reflex::Detail::ScopeOf <StateImpl*,true> ScopeOf;


	StateImpl(Bindings & bindings, Object & client);

	StateImpl(const StateImpl & state, Object & client);

	StateImpl(const StateImpl & state) = delete;


	virtual TRef <StateImpl> Clone(UInt8 contextflags, Object & client) const override;

	virtual bool Instantiate(const Module & module) override;

	virtual CString::View RegisterStaticString(Key32 id, CString && string) override
	{
		if (auto pstring = m_tempsymbols.SearchValue(id))
		{
			return *pstring;
		}
		else
		{
			return m_tempsymbols.Set(id, std::move(string));
		}
	}

	virtual CString::View GetStaticString(Key32 id, bool assume) const override
	{
		REFLEX_ASSERT(m_tempsymbols.Search(id) || assume);

		return *m_tempsymbols.SearchValue(id, &REFLEX_NULL(CString));
	}

	virtual void RegisterResourceType(Key32 id, TypeRef type, File::ResourcePool::Ctr ctr, const WString::View & ext, ConstTRef <Data::PropertySet> custom) override;

	virtual Symbol RegisterConstant(TypeRef typeref, Key32 ns, const CString::View & name, const Data::Archive::View & value) override;

	virtual void EnumerateConstants(const Reflex::Function <void(const Constant&)> & callback) const override { for (auto & i : m_constants) callback(i.value); }

	virtual Symbol RegisterTemplateType(Key32 ns, const CString::View & name, UInt ntarg, bool varadic, const Data::Archive::View & clientdata, TemplateType::Instantiator callback) override;

	virtual void EnumerateTemplateTypes(const Reflex::Function <void(const TemplateType&)> & callback) const override { for (auto & i : m_template_types) callback(i.value); }

	virtual Symbol RegisterTemplateFunction(Key32 ns, const CString::View & name, UInt32 ntarg, const ArrayView <Argument> & args, TemplateFunction::Instantiator callback, UInt8 flags) override;

	virtual void EnumerateTemplateFunctions(const Reflex::Function <void(const TemplateFunction&)> & callback) const override { for (auto & i : m_template_functions) callback(i.value); }

	virtual TypeRef InstantiateTemplateType(Symbol symbol, const ArrayView <TypeRef> & targs) override;

	virtual const InstantiatedTemplateType * GetInstantiatedTemplateType(TypeRef type) const override;

	virtual void RegisterTypedef(Symbol symbol, TypeRef type) override
	{
		m_typedefs.Acquire(ToUInt64(symbol)) = type;
	}

	virtual void EnumerateScriptFunctions(Symbol symbol, const Reflex::Function <void(const ScriptFunction&)> & callback) const override
	{
		for (auto & i : m_target->GetScriptFunctions(symbol))
		{
			callback(i.value);
		}
	}


	bool Compile(const WString::View & path, Scope & global);

	Data::ArchiveObject * ParseSource(const WString::View & path, const File::ResourcePool::Token * & ptoken);

	ArrayView <Key32> ParseSpec(Source & src, Itr & itr, const ArrayView <Key32> & valid);

	Tuple < Array<TypeRef>, Array<Key32> > & RetrieveInheritanceInfo(TypeRef type);

	bool InheritsFrom(TypeRef type, TypeRef base);


	REFLEX_INLINE CString::View AcquireAnonSymbolEx()
	{
		CString t;

		t.SetSize(9);

		t[0] = '~';

		CString::Region output = { t.GetData() + 1, 8 };

		Data::Detail::BytesToHex(output, Data::Pack(++m_anon));

		return AcquireStaticString(*this, std::move(t));
	}

	REFLEX_INLINE Key32 AcquireAnonSymbol()
	{
		return AcquireAnonSymbolEx();
	}

	String & AquireStringLiteral(Key64 hash, const CString::View & value)
	{
		auto & conststring = stringpool.value.Acquire(hash);

		if (!conststring) conststring = New<String>(value);

		return *conststring;
	}

	String & AquireStringLiteral(Key64 hash, const WString::View & value)
	{
		auto & conststring = stringpool.value.Acquire(hash);

		if (!conststring) conststring = New<String>(value);

		return *conststring;
	}

	const Constant * GetConstValue(Symbol symbol) const { return m_constants.SearchValue(ToUInt64(symbol)); }

	const TemplateType * GetTemplateType(Symbol symbol) const { return m_template_types.SearchValue(ToUInt64(symbol)); }

	bool IsAuto(TypeRef type) { return type == m_auto_placeholder; }

	void ThrowCustomError(Source & src, const CString::View & tmpl, const ArrayView <CString::View> & params);


	ConstTRef <Compiler> m_compiler;

	Object & client;

	TRef <Program> m_target;

	Array <Key32> m_modules;

	Array < Tuple < Key32, TypeRef, File::ResourcePool::Ctr, WString::View, ConstReference <Data::PropertySet> > > m_resourcetypes;

	Sequence <Key32,CString> m_tempsymbols;

	Sequence <UInt64,Constant> m_constants;

	Sequence <UInt64,TemplateType> m_template_types;

	Sequence <UInt64,TemplateFunction> m_template_functions;

	Sequence <TypeRef,InstantiatedTemplateType> m_instantiated_template_types;

	Sequence <UInt64,TypeRef> m_typedefs;


	Reference <Allocator> m_allocator;

	TypeRef m_auto_placeholder;

	TypeRef m_targ_placeholders[4];

	Sequence <Data::ArchiveObject*,bool> includeguard;

	Pair <WString::View> currentpath;


	StringPool & stringpool;


	UInt m_anon = 0;

	Sequence <ScriptFunction*,LayoutTemplate> scriptfunction_layouts;


	Scope * root = 0;

	Scope * current = 0;


	Sequence <UInt64,ScriptTemplateType> script_ttypes;

	Sequence <UInt64,ScriptTemplateFunction> script_tfns;



	//cache

	mutable CString symbol_cache;

	mutable Array <Argument> arguments_cache;

	mutable Data::Archive constant_cache;

	Sequence < TypeRef, Tuple < Array<TypeRef>, Array<Key32> > > inheritancechains_cache;

	Array <Key32> spec_cache;


	Sequence <UInt64,bool> instantiated_tfn_guard;

	Sequence <UInt64,Tuple<bool,UInt>> castable_cache;


	File::ResourcePool::Lock * filesystemlock;

	CString error_buffer;
};

struct CompilerImpl::ParserRestorePoint
{
	ParserRestorePoint(Itr & itr)
		: itr(itr),
		store(itr)
	{
	}

	void Rollback() { itr = store; }

	Itr & itr;

	const Itr store;
};

struct CompilerImpl::InstructionsRestorePoint
{
	InstructionsRestorePoint(Scope & scope);

	void Rollback();

	UInt GetNumInstruction() const;

	Scope & scope;


private:

	UInt m_ninstruction;

	UInt32 m_layout_size;

	UInt16 m_layout_nobject;

	UInt16 m_layout_nvarnames;

	UInt16 m_nvariables;

	UInt16 m_nusings;

};

struct CompilerImpl::RestorePoint :
	public ParserRestorePoint,
	public InstructionsRestorePoint
{
	RestorePoint(Scope & scope, Itr & itr);

	void Rollback();
};

struct CompilerImpl::ExternalConstsAccessor
{
	typedef NullType Param;

	typedef Array <const State::Constant*> Output;

	static bool Get(Scope & scope, Symbol name, const Param & flags, Output & map);
};

typedef CompilerImpl::ExternalConstsAccessor ExternalObjectAccessor;

REFLEX_INLINE Pair <CString::View> CompilerImpl::ParseNamespacedSymbol(Source & src, Itr & itr)
{
	return SplitNamespacedSymbol(ParseNamespaceChain(src, itr));
}

inline void CompilerImpl::StateImpl::ThrowCustomError(Source & src, const CString::View & tmpl, const ArrayView <CString::View> & params)
{
	constexpr CString::View placeholders[] = { "[0]", "[1]" };

	auto itr = placeholders;

	error_buffer = tmpl;

	REFLEX_FOREACH(p, params)
	{
		error_buffer = Replace(error_buffer, *itr++, p);
	}

	SyntaxError::Throw(src, error_buffer);
}


REFLEX_END_INTERNAL

REFLEX_SET_TRAIT(VM::Detail::CompilerImpl::BracketScope, IsBoolCastable);
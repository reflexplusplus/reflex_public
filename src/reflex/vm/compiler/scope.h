#pragma once

#include "compilerimpl.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

class CompilerImpl::Scope : public Object
{
public:

	enum SymbolType : UInt8
	{
		kSymbolTypeNone,

		kSymbolTypeType,

		kSymbolTypeVariable,

		kSymbolTypeConstant,
		//kSymbolTypeExternalVariable,

		//kSymbolTypeFunction,	//not ready for this yet
	};

	struct SymbolRef
	{
		SymbolType a;
		Value128 b;

		operator bool() { return a; }
	};

	struct SymbolAccessor;


	Scope(const Scope &) = delete;

	~Scope();


	Scope & GetRoot();

	bool IsGlobal() const { return m_global; }	//m_scopetype < kTypeFunction; }

	bool IsSub() const { return m_sub; }	//m_scopetype < kTypeFunction; }

	Key32 GetNamespace() const { return m_localns; }

	Scope * GetPrev() { return m_prev; }


	void RegisterUsing(Key32 ns)
	{
		REFLEX_ASSERT(IsValidKey(ns));

		Remove(m_usings, ns);

		m_usings.Push(ns);
	}

	void RegisterUseGlobal()
	{
		Remove(m_usings, kGlobal);

		m_usings.Push(kGlobal);
	}

	ArrayView <Key32> GetUsings() const { return m_usings; }


	bool RegisterTypedef(Key32 symbol, TypeRef type)
	{
		REFLEX_ASSERT(RemoveConst(type)->GetAllocator());

		auto & ptype = state.m_typedefs.Acquire(ToUInt64({ m_localns, symbol }));

		if (ptype) return ptype == type;

		REFLEX_ASSERT(!ptype);

		ptype = type;

		return true;
		//auto ok = bindings.RegisterTypeAlias({ m_localns, symbol }, type);

		//REFLEX_ASSERT(ok);

		//return ok;
	}


	CString::View ResolveNamespace(Source & src, const CString::View & value) const;


	const Variable & RegisterVariable(const Source & src, Key32 name, TypeRef type);

	Variable SearchVariable(Symbol symbol) const;


	TypeRef ParseType(Source & src, Itr & itr/*, bool expect = true*/);

	TypeRef ParseTypeOrAuto(Source & src, Itr & itr);


	void DispatchExitKeyword(Key32 keyword, Source & src, Itr & itr);



	//helpers

	template <class ACCESSOR> inline typename ACCESSOR::Output TraverseSymbolsOfType(Source & src, Itr & itr, const typename ACCESSOR::Param & param = {})
	{
		auto store = itr;

		auto pair = ParseNamespacedSymbol(src, itr);

		auto candidates = CompilerImpl::GetSymbolsOfType<ACCESSOR>(*this, pair.a, pair.b, m_usings, param);

		if (!candidates) { itr = store; }

		return candidates;
	}

	template <class OUTPUT> inline bool ParseTArgs(Source & src, Itr & itr, OUTPUT & output)
	{
		if (Traverse<Token::kTypeSymbol>(src, itr, kSymbolAt))
		{
			if (auto bracketscope = BracketScope(src, itr, kBracketRound))
			{
				output.AddTArg(ExpectType(src, bracketscope.inner, *this));

				while (bracketscope.inner)
				{
					Assume(Traverse<Token::kTypeSymbol>(src, bracketscope.inner, kSymbolComma), SyntaxError(src));

					output.AddTArg(ExpectType(src, bracketscope.inner, *this));
				}
			}
			else
			{
				output.AddTArg(ExpectType(src, itr, *this));
			}

			Traverse<Token::kTypeSymbol>(src, itr, kSymbolColon);

			return true;
		}

		return false;
	}

	void ThrowCustomError(Source & src, const CString::View & tmpl, const ArrayView <CString::View> & params);



	//links

	StateImpl & state;

	Reflex::Allocator & allocator;

	Bindings & bindings;

	LayoutTemplate & stackframelayout;

	Array <Instruction> & instructions;

	Array < Pair <Symbol,Variable> > & variables;



protected:

	friend struct InstructionsRestorePoint;


	Scope(StateImpl & state, bool global, bool sub, Scope * parent, LayoutTemplate & stackframelayout, Array < Pair <Symbol,Variable> > & variables, Array <Instruction> & instructions, Key32 localns);

	virtual bool OnHandleExit(Key32 keyword, Source & src, Itr & itr) { ThrowCustomError(src, kErrorTemplateInvalidKeyword, { GetExitKeyword(keyword) }); return false; }


	Scope * m_prev = 0;


	Scope * m_parent;

	bool m_global, m_sub;


	Key32 m_localns;

	Array <Key32> m_usings;
};

struct CompilerImpl::SubScope : public Scope
{
public:

	SubScope(Scope & parent);

	SubScope(Scope & parent, Key32 ns);


protected:

	virtual bool OnHandleExit(Key32 keyword, Source & src, Itr & itr) override;
};

struct CompilerImpl::AnonScope : public SubScope
{
	AnonScope(Scope & parent);
};

struct CompilerImpl::NamespaceScope : public SubScope
{
	NamespaceScope(Scope & parent, const CString::View & ns);
};

struct CompilerImpl::Scope::SymbolAccessor	//CURRENTYLE VARIABLES AND EXTERNS
{
	typedef Key32 SymbolType;

	typedef SymbolRef Output;

	typedef Pair <Scope*> Param;

	static bool Get(Scope & scope, Symbol symbol, const Pair <Scope *> & scopes, Output & output);
};

inline CompilerImpl::Scope & CompilerImpl::Scope::GetRoot()
{
	auto itr = this;

	while (itr->m_sub) itr = itr->GetPrev();

	return *itr;
}

inline Variable CompilerImpl::Scope::SearchVariable(Symbol symbol) const
{
	Pair <Symbol, Variable> t;

	return SearchValue<KeyCompare>(variables, symbol, &t)->b;
}

inline TypeRef CompilerImpl::Scope::ParseTypeOrAuto(Source & src, Itr & itr)
{
	if (auto type = ParseType(src, itr))
	{
		return type;
	}
	else if (itr && itr->hash == kauto)
	{
		Inc(src, itr);

		return state.m_auto_placeholder;
	}

	return 0;
}

REFLEX_INLINE void CompilerImpl::Scope::ThrowCustomError(Source & src, const CString::View & tmpl, const ArrayView <CString::View> & params)
{
	state.ThrowCustomError(src, tmpl, params);
}

UInt CompilerImpl::InstructionsRestorePoint::GetNumInstruction() const { return scope.instructions.GetSize() - m_ninstruction; }

REFLEX_END_INTERNAL

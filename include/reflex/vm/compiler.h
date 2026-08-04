#pragma once

#include "program.h"	//TRANSITIONAL MERGING TBINDINGS ADN COMPILER




//
//Experimental API

namespace Reflex::VM
{

	class Compiler;

}




//
//Compiler

#define VM_DEFINE_OPERATOR(OP) REFLEX_DECLARE_KEY32(OP); static constexpr const char * OP = REFLEX_STRINGIFY(OP)

class Reflex::VM::Compiler : public Object
{
public:

	static Compiler & null;

	//types

	class State;

	using DebugListing = Array < Tuple <Key32, UInt, CString> >;


	//standard operators (for overloads)

	VM_DEFINE_OPERATOR(opCreate);

	VM_DEFINE_OPERATOR(opCast);

	VM_DEFINE_OPERATOR(opEqual);
	VM_DEFINE_OPERATOR(opInequal);
	VM_DEFINE_OPERATOR(opLessThan);
	VM_DEFINE_OPERATOR(opLessThanOrEqual);
	VM_DEFINE_OPERATOR(opGreaterThan);
	VM_DEFINE_OPERATOR(opGreaterThanOrEqual);
	VM_DEFINE_OPERATOR(opLogicalNot);
	VM_DEFINE_OPERATOR(opLogicalAnd);
	VM_DEFINE_OPERATOR(opLogicalOr);

	VM_DEFINE_OPERATOR(opBand);
	VM_DEFINE_OPERATOR(opBor);
	VM_DEFINE_OPERATOR(opXor);
	VM_DEFINE_OPERATOR(opShl);
	VM_DEFINE_OPERATOR(opShr);

	VM_DEFINE_OPERATOR(opPreInc);
	VM_DEFINE_OPERATOR(opPreDec);
	VM_DEFINE_OPERATOR(opPostInc);
	VM_DEFINE_OPERATOR(opPostDec);

	VM_DEFINE_OPERATOR(opInvert);
	VM_DEFINE_OPERATOR(opAdd);
	VM_DEFINE_OPERATOR(opSubtract);
	VM_DEFINE_OPERATOR(opMultiply);
	VM_DEFINE_OPERATOR(opDivide);
	VM_DEFINE_OPERATOR(opMod);

	VM_DEFINE_OPERATOR(opAddAssign);
	VM_DEFINE_OPERATOR(opSubtractAssign);
	VM_DEFINE_OPERATOR(opMultiplyAssign);
	VM_DEFINE_OPERATOR(opDivideAssign);

	VM_DEFINE_OPERATOR(opGet);
	VM_DEFINE_OPERATOR(opSet);

	VM_DEFINE_OPERATOR(opInvoke);

	VM_DEFINE_OPERATOR(opBegin);
	VM_DEFINE_OPERATOR(opNext);

	VM_DEFINE_OPERATOR(opCreateArray);
	VM_DEFINE_OPERATOR(opCreateMap);
	VM_DEFINE_OPERATOR(opCreatePropertySet);



	//lifetime

	[[nodiscard]] static TRef <Compiler> Create();



	//access

	[[nodiscard]] virtual TRef <State> CreateState(UInt8 contextflags) const = 0;	//for prebindings optimisation

	[[nodiscard]] virtual TRef <Program> Compile(File::ResourcePool::Lock & lock, const WString::View & path, UInt8 contextflags, Object & clientdata = REFLEX_NULL(Object), const ArrayView <ConstTRef<Module>> & default_modules = {}, const State & prebindings = GetNullCompilerState()) const = 0;



protected:

	static const State & GetNullCompilerState();

};




//
//Compiler::State

class Reflex::VM::Compiler::State : public Reflex::Object
{
public:

	static State & null;

	//types

	using ModuleInstantiator = FunctionPointer <void(State&, UInt8)>;

	using ClientData = UInt8[32];

	struct Item
	{
		Symbol symbol;
		StaticString name;
	};

	struct Constant : public Item
	{
		TypeRef type;
		Pair <UInt64> value;
	};

	struct TemplateType : public Item
	{
		using Instantiator = FunctionPointer <TypeRef(State &, const ClientData clientdata, Key32, const StaticString &, const ArrayView<TypeRef> &)>;

		UInt8 ntarg;	//max ntarg
		bool varadic;
		ClientData clientdata;
		Instantiator instantiate;
	};

	struct TemplateFunction : public Item
	{
		using Instantiator = FunctionPointer <bool(State &, Key32, const StaticString &, const ArrayView <Argument> &, const ArrayView <Argument> &)>;	//Symbol,targs,args

		UInt8 flags;
		UInt32 ntarg;
		Array <Argument> arguments;
		Instantiator instantiate;
	};

	struct InstantiatedTemplateType
	{
		Symbol symbol;
		TypeRef type;
		Array <TypeRef> targs;
	};



	//lifetime

	State(Bindings & bindings);

	~State();



	//modules

	virtual bool Instantiate(const Module & module) = 0;



	//setup

	virtual StaticString RegisterStaticString(Key32 id, CString && string) = 0;

	virtual StaticString GetStaticString(Key32 id, bool assume = false) const = 0;


	virtual void RegisterResourceType(Key32 id, TypeRef type, File::ResourcePool::Ctr ctr, const WString::View & ext = {}, ConstTRef <Data::PropertySet> custom = {}) = 0;


	virtual Symbol RegisterConstant(TypeRef typeref, Key32 ns, const StaticString & name, const Data::Archive::View & value) = 0;

	virtual void EnumerateConstants(const Reflex::Function <void(const Constant&)> & callback) const = 0;


	virtual Symbol RegisterTemplateType(Key32 ns, const StaticString & name, UInt ntarg, bool varadic, const Data::Archive::View & clientdata, TemplateType::Instantiator callback) = 0;

	virtual void EnumerateTemplateTypes(const Reflex::Function <void(const TemplateType&)> & callback) const = 0;


	virtual Symbol RegisterTemplateFunction(Key32 ns, const StaticString & name, UInt32 ntarg, const ArrayView <Argument> & args, TemplateFunction::Instantiator callback, UInt8 flags = 0) = 0;

	virtual void EnumerateTemplateFunctions(const Reflex::Function <void(const TemplateFunction&)> & callback) const = 0;


	virtual TypeRef InstantiateTemplateType(Symbol symbol, const ArrayView <TypeRef> & targs) = 0;

	virtual const InstantiatedTemplateType * GetInstantiatedTemplateType(TypeRef type) const = 0;


	virtual void RegisterTypedef(Symbol symbol, TypeRef type) = 0;


	virtual void EnumerateScriptFunctions(Symbol symbol, const Reflex::Function <void(const ScriptFunction&)> & callback) const = 0;



	//links

	const TRef <Bindings> bindings;

};

REFLEX_SET_TRAIT(Reflex::VM::Compiler::State, IsSingleThreadExclusive);




//
//Detail

inline const Reflex::VM::Compiler::State & Reflex::VM::Compiler::GetNullCompilerState() { return State::null; }

REFLEX_NS(Reflex::VM)

inline StaticString AcquireStaticString(Compiler::State & state, CString && symbol)
{
	return state.RegisterStaticString(symbol, std::move(symbol));
}

template <bool RETAIN = true> inline void RegisterExternalObject(Compiler::State & b, TypeRef type, Key32 ns, const StaticString & name, Object & object)
{
	auto symbol = b.RegisterConstant(type, ns, name, Data::Pack(&object));

	if constexpr (RETAIN)
	{
		b.bindings->SetProperty(Reflex::Detail::MergeHashes(symbol.a.value, symbol.b.value), object);
	}
}

REFLEX_END

REFLEX_NS(Reflex::VM::Detail)

TRef <Object> OpenSourceFile(const File::ResourcePool::StreamContext & ctx, System::FileHandle & instream);

void AddSource(Program & program, String & path, TypeID type_id, Object & source);

REFLEX_INLINE CString MakeNamespacedSymbol(const Compiler::State & bindings, Key32 ns, const StaticString & name, bool assume = false)
{
	if (IsValidKey(ns))
	{
		return Join(bindings.GetStaticString(ns, assume), kNamespaceDelimiter, name);
	}
	else
	{
		return name;
	}
}

REFLEX_INLINE CString MakeNamespacedSymbol(const Compiler::State & bindings, TypeRef type)
{
	return MakeNamespacedSymbol(bindings, type->symbol.a, type->name);
}

REFLEX_INLINE StaticString GetNamespaceString(const Compiler::State & tbindings, Key32 ns)
{
	return tbindings.GetStaticString(ns, true);
}

inline Reflex::Detail::DynamicTypeRef AcquireObjectType(const Compiler::State & tbindings, Reflex::Detail::DynamicTypeRef base, Key32 ns, StaticString name, const WString::View & filepath = {})
{
	return VM::AcquireObjectType(base, MakeNamespacedSymbol(tbindings, ns, name), filepath);
}

CString MakeTemplateName(Compiler::State & tbindings, StaticString name, const ArrayView <TypeRef> & targs);

REFLEX_END

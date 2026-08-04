#pragma once

#include "programimpl.h"
#include "compiler/compilerimpl.h"




//
//

REFLEX_NS(Reflex::VM)

struct Library : public Object
{
	struct NullString : public String
	{
		NullString()
			: String(kNullKey, 0),
			pad(0)
		{
		}

		WChar pad;
	};

	struct NullBindings : public Bindings
	{
		using Bindings::Bindings;
	};

	struct NullCompiler : public Compiler
	{
		NullCompiler() {}

		virtual void RegisterResourceType(Key32 id, TypeRef type, File::ResourcePool::Ctr ctr, const WString::View & ext) {}

		virtual TRef <State> CreateState(UInt8 contextflags) const override { return REFLEX_NULL(State); }

		virtual TRef <Program> Compile(File::ResourcePool::Lock & lock, const WString::View & path, UInt8 contextflags, Object & clientdata, const ArrayView <ConstTRef<Module>> & default_modules, const State & prebindings) const override { return REFLEX_NULL(Program); }
	};

	struct NullCompilerState : public Detail::CompilerImpl::CloneableState
	{
		using Detail::CompilerImpl::CloneableState::CloneableState;

		virtual bool Instantiate(const Module & module) override { return false; }

		virtual void RegisterResourceType(Key32 id, TypeRef type, File::ResourcePool::Ctr ctr, const WString::View & ext, ConstTRef <Data::PropertySet> options) override {}

		virtual CString::View RegisterStaticString(Key32 id, CString && string) override { return {}; }

		virtual CString::View GetStaticString(Key32 id, bool assume = false) const override { return {}; }

		virtual Symbol RegisterConstant(TypeRef typeref, Key32 ns, const CString::View & name, const Data::Archive::View & value) override { return {}; }

		virtual void EnumerateConstants(const Reflex::Function <void(const Constant&)> & callback) const override {};

		virtual Symbol RegisterTemplateType(Key32 ns, const CString::View & name, UInt ntarg, bool varadic, const Data::Archive::View & clientdata, TemplateType::Instantiator callback) override { return {}; }

		virtual void EnumerateTemplateTypes(const Reflex::Function <void(const TemplateType&)> & callback) const override {};

		virtual Symbol RegisterTemplateFunction(Key32 ns, const CString::View & name, UInt32 ntarg, const ArrayView <Argument> & args, TemplateFunction::Instantiator callback, UInt8 flags) override { return {}; }

		virtual void EnumerateTemplateFunctions(const Reflex::Function <void(const TemplateFunction&)> & callback) const override {};

		virtual TypeRef InstantiateTemplateType(Symbol symbol, const ArrayView <TypeRef> & targs) override { return {}; }

		virtual const InstantiatedTemplateType * GetInstantiatedTemplateType(TypeRef type) const override { return {}; }

		virtual void RegisterTypedef(Symbol symbol, TypeRef type) override { }

		virtual void EnumerateScriptFunctions(Symbol symbol, const Reflex::Function <void(const ScriptFunction&)> & callback) const override {}

		virtual TRef <Detail::CompilerImpl::StateImpl> Clone(UInt8 contextflags, Object & client) const override;
	};

	Library();

	~Library();


	Reference <System::CriticalSection> m_cs;

	Reference <Reflex::Allocator> m_allocator_retainer;

	Reflex::Allocator & allocator;

	NullString string, doublequote;

	Detail::Token token;

	Sequence <Key32,Detail::Token::Type> keywords;


	struct Bytes { UInt8 data[sizeof(Reflex::Detail::DynamicTypeInfo)]; };

	Sequence < TypeID, Tuple <TypeID, CString, Bytes> > m_psuedoclasses;


	NullBindings bindings;

	NullCompiler compiler;

	NullCompilerState compilestate;

	Detail::ProgramImpl exe;


	Detail::ContextImpl::Token::List m_contexts;

	Detail::ContextImpl context;
};

void LogInstruction(LogType type, Context & context, CString && msg);

typedef Reflex::The <Library> TheLibrary;

REFLEX_END

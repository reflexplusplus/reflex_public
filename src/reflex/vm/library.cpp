#include "library.h"

#include "compiler/compilerimpl.h"

#include "bindings/core/array.h"




//
//

#define VM_INTRINSIC_INFO(INTRINSIC,NTARG,NARG) Detail::kIntrinsicInfo[OPCODE(INTRINSIC)] = { false, NTARG, NARG }

REFLEX_BEGIN_INTERNAL(Reflex::VM)

template <class TYPE> void RegisterValue(Bindings & bindings, TypeID type_id, CString::View name, UInt size)
{
	UInt32 memory = 0;

	auto null = Data::Pack(memory);

	REFLEX_CREATE(Type, bindings, type_id, kGlobal, name, Splice(null, size).a);
}

REFLEX_END_INTERNAL

Reflex::VM::Library::Library()
	: m_allocator_retainer(CreateAllocator(kRecycleAllocID, REFLEX_NULL(Object))),
	allocator(m_allocator_retainer),
	bindings([]() -> UInt8
	{ 
		The<Library>::st_initalised = true;
	
		return 0; 
	}()),
	exe(bindings),
	compilestate(bindings),
	context(0),
	m_cs(System::CriticalSection::Create(false))
{
	doublequote.data[0] = WChar(kDoubleQuote);
	RemoveConst(doublequote.size) = 1;
	RemoveConst(doublequote.hash) = ToView(kDoubleQuote);

	RemoveConst(exe.sources).Push();

	typedef Detail::CompilerImpl Compiler;

	keywords.Insert(Compiler::knamespace, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::ktemplate, Detail::Token::kTypeKeyword);
	//keywords.Insert(Compiler::kInterface, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kmethod, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kobject, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kstruct, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kobject, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kstatic, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kconst, Detail::Token::kTypeKeyword);
	//keywords.Insert(Compiler::kAuto, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kfor, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kwhile, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kforeach, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kif, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kelse, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kswitch, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kcase, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kbreak, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kcontinue, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kreturn, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kexit, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kDefault, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::knull, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::knew, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kbind, Detail::Token::kTypeKeyword);

	keywords.Insert(Compiler::kPublic, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kProtected, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::kPrivate, Detail::Token::kTypeKeyword);

	keywords.Insert(Compiler::kInline, Detail::Token::kTypeKeyword);

	keywords.Insert(Compiler::kusing, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::ktypedef, Detail::Token::kTypeKeyword);
	keywords.Insert(Compiler::ktypeid, Detail::Token::kTypeKeyword);

	exe.Invalidate();

	context.Initialise(exe, REFLEX_NULL(Object));
}

Reflex::VM::Library::~Library()
{
	REFLEX_ASSERT(exe.sources.GetSize() == 1);
}

Reflex::TRef <Reflex::VM::Detail::CompilerImpl::StateImpl> Reflex::VM::Library::NullCompilerState::Clone(UInt8 contextflags, Object & client) const
{
	return REFLEX_CREATE(Detail::CompilerImpl::StateImpl, New<Bindings>(contextflags), client);
}

Reflex::VM::Detail::Token & Reflex::VM::Detail::Token::null = Reflex::The<VM::Library>::Get<true>()->token;

#define REFLEX_VM_NULL(TYPE, x) Reflex::VM::TYPE & Reflex::VM::TYPE::null = Reflex::The<Reflex::VM::Library>::Get<true>()->x

REFLEX_VM_NULL(String, string);
REFLEX_VM_NULL(Bindings, bindings);
REFLEX_VM_NULL(Program, exe);
REFLEX_VM_NULL(Compiler, compiler);
REFLEX_VM_NULL(Compiler::State, compilestate);
REFLEX_VM_NULL(Context, context);

Reflex::TRef <Reflex::Object> Reflex::VM::Start()
{
	return TheLibrary::Acquire();
}

Reflex::TypeID Reflex::VM::GenerateScriptTypeID(const CString::View & symbol, const WString::View & path)
{
	TypeID type_id = Reflex::Detail::MakeHash<UInt32>(symbol);

	REFLEX_FOREACH(c, path) Reflex::Detail::IncrementHash(type_id, c);

	return type_id;
}

Reflex::Detail::DynamicTypeRef Reflex::VM::AcquireObjectType(Reflex::Detail::DynamicTypeRef base, const CString::View & symbol, const WString::View & path)
{
	auto library = The<Library>::Get();

	auto type_id = GenerateScriptTypeID(symbol, path);

	System::CriticalSection::Lock lock(library->m_cs);

	REFLEX_ASSERT(type_id > Reflex::Detail::g_type_index_counter);

	if (auto existing = library->m_psuedoclasses.SearchValue(type_id))
	{
		auto & classinfo = *Reinterpret<Reflex::Detail::DynamicTypeInfo>(existing->c.data + 0);

		REFLEX_ASSERT(classinfo.base == base);

		REFLEX_ASSERT(classinfo.type_id == type_id);

		return &classinfo;
	}
	else
	{
		auto & info = library->m_psuedoclasses.Insert(type_id);

		info.a = type_id;

		info.b = symbol;

		return Reflex::Detail::Constructor<Reflex::Detail::DynamicTypeInfo>::Construct(info.c.data + 0, base, &info.a, info.b.GetData());
	}
}

const Reflex::VM::ScriptFunction * Reflex::VM::QueryFunction(const Program & program, Symbol symbol, Argument rtn, const ArrayView <Argument> & args)
{
	for (auto & i : program.GetScriptFunctions(symbol))
	{
		auto & fn = i.value;

		if (fn.rtn == rtn && fn.args.GetSize() == args.size)
		{
			auto pa = fn.args.GetData();

			auto pb = args.data;

			REFLEX_LOOP_PTR(pa, a, args.size)
			{
				if (*a != *pb++) goto Next;
			}

			return &fn;
		}

		REFLEX_MARKER(Next);
	}

	return nullptr;
}
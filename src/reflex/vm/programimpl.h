#pragma once

#include "[require].h"




#define INLINE(a,b) REFLEX_INLINE_LOCAL(a, b)
#define LOCAL(a,b) REFLEX_LOCAL(a, b)
#define END REFLEX_END

#define VM_PTRSIZE sizeof(void*)
#define VM_MODULO_PTRSIZE(a) (a & (sizeof(void*)-1))

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

inline static const bool kOptimise = true;

typedef Array <Instruction> Instructions;

extern CString::View kOpcodes[OPCODE(NumOpcode) + 1];

REFLEX_INLINE Pair <Int16,UInt16> GetVar(const Instruction & i) //-> adr + size
{
	return Reinterpret<Pair<Int16,UInt16>>(i.param32);
}

REFLEX_INLINE CString::View GetSymbolName(const Compiler::State & state, Key32 symbol)
{
	if (auto name = state.GetStaticString(symbol, true)) return name;

	return "{unknown}";
}

REFLEX_INLINE bool IsJump(UInt8 opcode)
{
	return Inside<UInt8>(opcode, OPCODE(Jump), (OPCODE(JumpIfTrue32) + 1) - OPCODE(Jump));
}

inline Source ToSource(const Instruction & instruction)
{
	return { instruction.line, instruction.file };
}

//enum DataType : UInt8
//{
//	kDataUnknown,
//	kDataArgument,
//	kDataMarker,
//	kDataFnCapture,
//};

typedef Tuple <const Instruction *, UInt16, UInt16> CurrentPositionStore;	//use this typedef to locate all places where this is used (affects layout)

REFLEX_STATIC_ASSERT(sizeof(CurrentPositionStore) == (sizeof(Instruction *) * 2));

inline const Int16 kPreStackframeDataSize = sizeof(CurrentPositionStore) + sizeof(Layout *);

inline Int16 GetArgumentsLocation(const ScriptFunction & fn)
{
	return -(Int16(fn.arguments_size) + kPreStackframeDataSize);
}

typedef Array < Tuple <UInt16, TypeRef, Key32> > DebugVarNames;

template <bool CTR>
struct LayoutTemplateImpl : public Object
{
	typedef UInt16 Offset;

	typedef Tuple <Offset,TypeRef,Type::Ctr> ScriptObjectObject;

	typedef typename Selector<CTR,ScriptObjectObject,Tuple<Offset>>::type ObjectInfo;


	UInt16 AddValue(const Type & type, Key32 name, const Data::Archive::View & value)
	{
		REFLEX_ASSERT(value.size == type.size);

		UInt16 offset = UInt16(init_state.GetSize());

		init_state.Append(value);

		varnames.Push({ offset, &type, name });

		return offset;
	}

	template <bool PAD> UInt16 AddObject(const Type & type, Key32 name, Object * object)
	{
		REFLEX_STATIC_ASSERT(CTR == false);

		UInt16 offset = UInt16(init_state.GetSize());

		auto pad = PAD ? UInt16(VM_MODULO_PTRSIZE(VM_PTRSIZE - VM_MODULO_PTRSIZE(offset))) : UInt16(0);

		init_state.Expand(pad);

		init_state.Append(Data::Pack(object));

		offset += pad;

		objects.Push({ offset });

		varnames.Push({ offset, &type, name });

		return offset;
	}

	UInt32 CalculateLayoutSize() const
	{
		return 4 + init_state.GetSize() + (objects.GetSize() * sizeof(ObjectInfo));
	}

	Layout * PackLayout(Data::Archive & archive) const
	{
		auto size = init_state.GetSize();

		auto totalsize = CalculateLayoutSize();

		archive.Allocate(archive.GetSize() + totalsize);

		auto raw = archive.GetData() + archive.GetSize();

		Data::Serialize(archive, MakeTuple(UInt16(size), UInt16(objects.GetSize())));

		archive.Append(init_state);

		for (auto & i : objects) archive.Append({ Reinterpret<UInt8>(&i), sizeof(i) });

		return Reinterpret<Layout>(raw);
	}

	Data::Archive init_state;

	Array <ObjectInfo> objects;

	DebugVarNames varnames;
};

using LayoutTemplate = LayoutTemplateImpl <false>;

CString GetVarDesc(const Compiler::State & bindings, const DebugVarNames::Type & var)
{
	return Join('[', var.b->name, kSpace, '"', GetSymbolName(bindings, var.c), '"', kSpace, '&', ToCString(var.a), ']');
}

DebugVarNames::Type LookupVar(const Compiler::State & bindings, const LayoutTemplate & t, UInt16 adr)
{
	DebugVarNames::Type null = { kMaxUInt16, bindings.bindings->void_t, kNullKey };

	return *SearchValue<KeyCompare>(t.varnames, adr, &null);
}

CString GetVarDesc(const Compiler::State & bindings, const LayoutTemplate & t, UInt16 adr)
{
	return GetVarDesc(bindings, LookupVar(bindings, t, adr));
}

struct SwitchTable : public Object
{
	REFLEX_OBJECT(SwitchTable,Object);

	Array <Instruction> * pinstructions;

	Array < Pair<UInt32,UIntNative> > cases;	//constant -> marker
};

struct ContextImpl;

struct ProgramImpl : public Program
{
	REFLEX_INLINE static CString::View InitOpcode(Opcode opcode, const CString::View & label) { return kOpcodes[opcode] = label; }

	ProgramImpl(const Bindings & program);

	~ProgramImpl();


	virtual Sequence<UInt64,ScriptFunction>::ConstRange GetScriptFunctions(Symbol symbol) const override;

	virtual operator bool() const override { return status; }


	void Optimise(const Compiler::State & state, LayoutTemplate & global, Instructions & global_code, Sequence <ScriptFunction*,LayoutTemplate> & functions);

	void Link(const Compiler::State & state, LayoutTemplate & global, Sequence <ScriptFunction*,LayoutTemplate> & function_layouts);

	void List(const Compiler::State & state, LayoutTemplate & global_layout_tmpl, Sequence <ScriptFunction*,LayoutTemplate> & function_layouts_tmpls);

	void Store(LayoutTemplate & global, Sequence <ScriptFunction*, LayoutTemplate> & function_layouts);

	void Invalidate();	//call if cant link


	template <bool GLOBAL, class TYPE> static TYPE & GetVarAtLocation(ContextImpl & context, Int location);

	template <bool GLOBAL, class TYPE> static TYPE & GetVar(ContextImpl & context, const Instruction & instruction);


	template <bool OBJECTS> static void PushStackFrame(ContextImpl & context, const Layout & init);

	template <bool OBJECTS, bool LOCAL = true> static void PopStackFrame(ContextImpl & context, const Layout & init);


	template <bool GLOBAL> static void PushValueGeneric(ContextImpl & context, const Instruction & instruction);

	template <bool GLOBAL> static void AssignGeneric(ContextImpl & context, const Instruction & instruction);


	template <bool GLOBAL, class TYPE> static void PushValue(ContextImpl & context, const Instruction & instruction);

	template <bool GLOBAL, class TYPE> static void PushValuePair(ContextImpl & context, const Instruction & instruction);

	template <bool GLOBAL> static void PushValue64and32(ContextImpl & context, const Instruction & instruction);

	template <bool GLOBAL, class TYPE> static void PushValueandConst32(ContextImpl & context, const Instruction & instruction);


	template <bool PUSH, bool GLOBAL, class TYPE> static void AssignValue(ContextImpl & context, const Instruction & instruction);

	template <bool PUSH, bool GLOBAL, class OBJECT> static void AssignObject(ContextImpl & context, const Instruction & instruction);


	static void Switch(ContextImpl & context, UInt value, const Instruction * & pinstruction);


	template <bool OBJECTS> static void CallFnImpl(ContextImpl & context, const Instruction * & pinstruction);

	template <class SIZE_HANDLER> static void ReturnImpl(ContextImpl & context, const Instruction * & pinstruction);

	template <class TYPE> static void AddAssign(ContextImpl & context);
	template <class TYPE> static void SubtractAssign(ContextImpl & context);
	template <class TYPE> static void MulAssign(ContextImpl & context);

	template <class TYPE> static void optimisedIntegralNext(ContextImpl & context);



	#define OP(CODE) static const CString::View & REFLEX_CONCATENATE(kOpcode_,CODE); static void CODE(ContextImpl & context, const Instruction * & pinstruction);
	#include "../../../include/reflex/vm/detail/[opcodes].h"
	#undef OP


	//Array <Key32> modules;

	Sequence <UInt64,ScriptFunction> functions;

	Data::Archive layoutdata;

	Layout * global_layout;

	Array < Reference <Object> > data;

	Array <Instruction> global_instructions;

	DebugVarNames varnames;

	bool status;


	REFLEX_IF_DEBUG(Reference <ObjectOf<Compiler::DebugListing>,kReferenceUnsafe> m_debuglisting;)
};

struct ContextImpl : public Context
{
	struct Token : public Reflex::Item <Token, false>
	{
		Token(ContextImpl * context) : context(context) {}

		typedef Reflex::Item <Token, false> Item;

		using Item::Attach;

		using Item::Detach;

		ContextImpl * context;
	};


	ContextImpl(UInt16 contextid)
		: Context(contextid)
	{
		Retain(program);
	}

	~ContextImpl();


	virtual Object * GetNullByRTTID(UInt32 type_id, Object * fallback) const override;

	virtual void InitialiseNulls(const Program & program) override;

	virtual void InitialiseGlobals(Object & clientdata) override;

	virtual bool DoCall(const ScriptFunction & scriptfunction) override;	//need to load stack, use helper functions


	const String & GetCurrentSource() const
	{
		return program->sources[instructionptr->file].path;
	}

	virtual void AdoptCirculars(Context & context, Reflex::Detail::DynamicTypeRef object_t) override;


	bool Run(UInt stackframidx, const ScriptFunction * fn);

	void Abort();


	ConstReference <Program> m_executable;

	Sequence < TypeID, Reference <Object> > m_tid2null;

	UInt8 * m_global_stackframe;

	Flags8 m_initalised;

	Array <UInt8*> m_stackframes;

	UInt8 * m_current_stackframe;


	Detail::Stack m_return_buffer;
};

inline void ReleaseContextCirculars(UInt16 contextid, Object & object)
{
	if (object.GetContextID() == contextid)
	{
		object.ReleaseData();
	}
}

REFLEX_STATIC_ASSERT(sizeof(Instruction) == 16);

template <bool OBJECTS, bool CTR> REFLEX_INLINE void InitialiseLayout(ContextImpl & context, const Layout & layout, UInt8 * target)
{
	auto base = Reinterpret<UInt8>(&layout) + 4;

	MemCopy(base, target, layout.size);

	if constexpr (OBJECTS)
	{
		auto objects = Reinterpret<typename LayoutTemplateImpl<CTR>::ObjectInfo>(base + layout.size);

		REFLEX_LOOP_PTR(objects, itr, layout.num_object)
		{
			auto & info = *itr;

			Pointer & pointer = *Reinterpret<Pointer>(target + info.a);

			if constexpr (CTR)
			{
				pointer = &info.c(context, info.b);
			}

			pointer->RetainMt();
		}
	}
}

template <bool OBJECTS, bool CTR> REFLEX_INLINE void DestroyLayout(const Layout & layout, UInt8 * target)
{
	if constexpr (OBJECTS)
	{
		auto base = Reinterpret<UInt8>(&layout) + 4;

		auto objects = Reinterpret<typename LayoutTemplateImpl<CTR>::ObjectInfo>(base + layout.size);

		REFLEX_RLOOP_PTR(objects, itr, layout.num_object)
		{
			auto & info = *itr;

			Pointer object = *Reinterpret<Pointer>(target + info.a);

			object->ReleaseMt();
		}
	}
}

REFLEX_END_INTERNAL;

REFLEX_ASSERT_RAW(Reflex::VM::Detail::Instruction);




//
//intrinsics

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

struct Int32Iterator : public Object
{
	REFLEX_OBJECT(Int32Iterator, Object);

	Int32Iterator(Int32 n = 0) : m_n(Max<Int32>(n,0)), m_value(0) {}

	Int32 m_n;

	Int32 m_value;
};

REFLEX_END_INTERNAL

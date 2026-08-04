#include "programimpl.h"
#include "bindings/core/arrayiterator.h"




//
//before the stackframe, reserve pointer size, this is used in abort
//take care of compiler function arguments, these are addressed negative to stackframe
//
//
//stackframe
//[simple values]
//[pointers]




//
//

REFLEX_STATIC_ASSERT(sizeof(Reflex::VM::Detail::Instruction) == 16);

REFLEX_STATIC_ASSERT(Reflex::VM::kNumOpcode < 256);

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

const UInt NumRemainingOpcodes = 256 - Reflex::VM::kNumOpcode;

template <class RTN> using ObjectMethodPointerType_0P = RTN(Object::*)();
template <class RTN, class P1> using ObjectMethodPointerType_1P = RTN(Object::*)(P1);
template <class RTN, class P1, class P2> using ObjectMethodPointerType_2P = RTN(Object::*)(P1, P2);

CString::View kOpcodes[OPCODE(NumOpcode) + 1];

struct ValueReturnHandler
{
	REFLEX_INLINE ValueReturnHandler(ContextImpl & context, const ScriptFunction & fn)
		: rtn_size(fn.rtn.type->size),
		rtn(ToView(Extend<kAllocateNone>(context.m_return_buffer, rtn_size)))
	{
		REFLEX_ASSERT(rtn_size == context.m_return_buffer.GetSize());

		MemCopy(Pop(context.stack, rtn.size), RemoveConst(rtn.data), rtn.size);	//!copy, because destructors called when releasing stack frame can manipulate stack
	}

	REFLEX_INLINE void Apply(ContextImpl & context)
	{
		context.stack.Append<kAllocateNone>(rtn);

		context.m_return_buffer.Shrink(rtn_size);
	}

	UInt rtn_size;

	Data::Archive::View rtn;
};

struct VoidReturnHandler
{
	REFLEX_INLINE VoidReturnHandler(ContextImpl & context, const ScriptFunction & fn) {}

	REFLEX_INLINE void Apply(ContextImpl & stack) {}
};

template <class TYPE>
struct SizedReturnHandler
{
	REFLEX_INLINE SizedReturnHandler(ContextImpl & context, const ScriptFunction & fn)
		: temp(Pop<TYPE>(context.stack))
	{
	}

	REFLEX_INLINE void Apply(ContextImpl & context)
	{
		Push(context.stack, temp);
	}

	TYPE temp;
};

template <class OBJECT>
struct ObjectReturnHandler
{
	REFLEX_INLINE ObjectReturnHandler(ContextImpl & context, const ScriptFunction & fn)
		: object(Pop<OBJECT&>(context.stack))
	{
		Retain(object);
	}

	REFLEX_INLINE void Apply(ContextImpl & context)
	{
		Push(context.stack, Cast<Object>(&object));

		Reflex::Detail::ReleaseSilent(object);
	}

	OBJECT & object;
};

typedef ObjectST * PointerST;

ProgramImpl::ProgramImpl(const Bindings & bindings)
	: Program(bindings)
	, status(false)
	REFLEX_IF_DEBUG(, m_debuglisting(REFLEX_CREATE(ObjectOf<Compiler::DebugListing>)))
{
	Retain(bindings);

	global_instructions.Allocate(128);

	layoutdata.SetSize(4);

	layoutdata.Wipe();

	global_layout = Reinterpret<Detail::Layout>(layoutdata.GetData());
}

ProgramImpl::~ProgramImpl()
{
	Release(bindings);
}

Sequence<UInt64,ScriptFunction>::ConstRange ProgramImpl::GetScriptFunctions(Symbol symbol) const
{
	auto & symbolid = Reinterpret<UInt64>(symbol);

	return MakeRange(functions, symbolid, symbolid + 1);
}

void ProgramImpl::Invalidate()
{
	functions.Clear();

	data.Clear();

	global_instructions.Clear();

	global_instructions.Push({ kMaxUInt16, 0, OPCODE(End), 0, 0 });	//in case last item is a marker

	layoutdata.SetSize(4);

	layoutdata.Wipe();

	global_layout = Reinterpret<Detail::Layout>(layoutdata.GetData());

	status = false;

	if (!sources) RemoveConst(sources).Push();
}

void ProgramImpl::Store(LayoutTemplate & global, Sequence <ScriptFunction*, LayoutTemplate> & function_layouts)
{
}

void ProgramImpl::Link(const Compiler::State & state, LayoutTemplate & global_layout_tmpl, Sequence <ScriptFunction*,LayoutTemplate> & function_layouts_tmpls)
{
	struct MarkerCompare { static bool eq(const Instruction & instruction, UInt32 markerid) { return And(instruction.opcode == OPCODE(Marker), instruction.param64 == UInt32(markerid)); } };

	typedef Sequence <const Instruction*,Key32> JumpInfo;

	LOCAL(void, Finalise)(const Bindings & bindings, Array <Instruction> & instructions, JumpInfo & jumps_dbg)
	{
		auto pinstructions = instructions.GetData();

		for (auto & i : instructions)
		{
			switch (i.opcode)
			{
			case OPCODE(BindFnObject):
				if (ToPointer<Function>(UIntNative(i.param64))->type != Function::kTypeScript) break;

			case OPCODE(CallFn):
			case OPCODE(optimisationCallFnOfValues):
			{
					auto & fn = *ToPointer<ScriptFunction>(UIntNative(i.param64));

					if (!fn.instructions)
					{
						Error error = { { i.line, i.file }, kErrorStageLink, "function not defined" };

						throw(error);
					}
				}
				break;

			case OPCODE(Jump):
			case OPCODE(JumpIfFalse8):
			case OPCODE(JumpIfTrue8):
			case OPCODE(JumpIfFalse32):
			case OPCODE(JumpIfTrue32):
				{
					auto & markerid = Reinterpret<UInt32>(i.param64);

					auto idx = Search<MarkerCompare>(instructions, markerid);

					REFLEX_ASSERT(idx);

					Reinterpret<Instruction*>(i.param64) = pinstructions + idx.value + 1;

					jumps_dbg.Acquire(&i) = markerid;
				}
				break;

			default:
				REFLEX_ASSERT(!IsJump(i.opcode));
				break;
			}
		}
	}
	END



	REFLEX_ASSERT(sources);

	global_instructions.Push({ kMaxUInt16, 0, OPCODE(End), 0, 0 });	//in case last item is a marker



	//layouts

	UInt layouts_size = global_layout_tmpl.CalculateLayoutSize();

	REFLEX_FOREACH(itr, function_layouts_tmpls) layouts_size += itr.value.CalculateLayoutSize();

	layoutdata.Allocate(layouts_size);

	layoutdata.Clear();


	global_layout = global_layout_tmpl.PackLayout(layoutdata);

	REFLEX_FOREACH(itr, function_layouts_tmpls)
	{
		itr.key->layout = itr.value.PackLayout(layoutdata);
	}



	//complete instructions

	JumpInfo jumps_dbg;

	//LinkExternalObjects::Call(bindings, global_instructions);

	//Optimise::Call(bindings, global_instructions);

	Finalise::Call(bindings, global_instructions, jumps_dbg);

	REFLEX_FOREACH(itr, functions)
	{
		auto & fn = itr.value;

		//LinkExternalObjects::Call(bindings, fn.instructions);

		//Optimise::Call(bindings, fn.instructions);

		Finalise::Call(bindings, fn.instructions, jumps_dbg);
	}

	REFLEX_FOREACH(itr, data)
	{
		if (auto table = DynamicCast<SwitchTable>(*itr))
		{
			auto & instructions = *table->pinstructions;

			auto pinstructions = instructions.GetData();

			REFLEX_FOREACH(item, table->cases)
			{
				auto idx = Search<MarkerCompare>(instructions, UInt32(item.b));

				REFLEX_ASSERT(idx);

				//TODO OPTIMISE dont need this + 1 ?

				Reinterpret<Instruction*>(item.b) = pinstructions + idx.value + 1;
			}
		}
	}

	status = true;
}

template <bool GLOBAL, class TYPE> REFLEX_INLINE TYPE & ProgramImpl::GetVarAtLocation(ContextImpl & context, Int location)
{
	if (GLOBAL)
	{
		return *Reinterpret<TYPE>(context.m_global_stackframe + location);
	}
	else
	{
		REFLEX_ASSERT(context.m_stackframes.GetLast() == context.m_current_stackframe);

		return *Reinterpret<TYPE>(context.m_current_stackframe + location);
	}
}

template <bool GLOBAL, class TYPE> REFLEX_INLINE TYPE & ProgramImpl::GetVar(ContextImpl & context, const Instruction & instruction)
{
	return GetVarAtLocation<GLOBAL,TYPE>(context, Reinterpret<Int16>(instruction.param32));
}

template <bool GLOBAL> REFLEX_INLINE void ProgramImpl::PushValueGeneric(ContextImpl & context, const Instruction & instruction)
{
	//THIS CAN CRASH
	//the data is read from stack
	//if stack is expanded, that data becomes invalid;

	auto info = Reinterpret<Pair<Int16, UInt16>>(instruction.param32);

	Data::Archive::View ref = { &GetVarAtLocation<GLOBAL,UInt8>(context, info.a), info.b };

	//auto t = Data::Unpack<Pointer>(ref);

	context.stack.Append<kAllocateNone>(ref, true);
}

template <bool GLOBAL, class TYPE> REFLEX_INLINE void ProgramImpl::PushValue(ContextImpl & context, const Instruction & instruction)
{
	Push(context.stack, Copy(GetVar<GLOBAL,TYPE>(context, instruction)));
}

template <bool GLOBAL, class TYPE> REFLEX_INLINE void ProgramImpl::PushValuePair(ContextImpl & context, const Instruction & instruction)
{
	auto a = GetVarAtLocation<GLOBAL,TYPE>(context, Reinterpret<Int16>(instruction.param32));
	auto b = GetVarAtLocation<GLOBAL,TYPE>(context, Reinterpret<Pair<Int16>>(instruction.param32).b);

	Push(context.stack, MakeTuple(a,b));
}

template <bool GLOBAL> REFLEX_INLINE void ProgramImpl::PushValue64and32(ContextImpl & context, const Instruction & instruction)
{
	auto & pair = *Reinterpret<Pair<Value64,Value32>>(Extend(context.stack, 12).data);

	pair.a = GetVarAtLocation<GLOBAL,Value64>(context, Reinterpret<Int16>(instruction.param32));

	pair.b = GetVarAtLocation<GLOBAL,Value32>(context, Reinterpret<Pair<Int16>>(instruction.param32).b);
}

template <bool GLOBAL,class TYPE> REFLEX_INLINE void ProgramImpl::PushValueandConst32(ContextImpl & context, const Instruction & i)
{
	auto & pair = *Reinterpret<Pair<TYPE,Value32>>(Extend(context.stack, sizeof(TYPE) + sizeof(Value32)).data);

	pair.a = GetVarAtLocation<GLOBAL,TYPE>(context, Reinterpret<Int16>(i.param32));

	pair.b = Reinterpret<Value32>(i.param64);
}

template <bool GLOBAL> REFLEX_INLINE void ProgramImpl::AssignGeneric(ContextImpl & context, const Instruction & instruction)
{
	auto info = Reinterpret<Pair<Int16, UInt16>>(instruction.param32);

	auto bytes = Pop(context.stack, info.b);

	auto var = &GetVarAtLocation<GLOBAL, UInt8>(context, info.a);

	MemCopy(bytes, var, info.b);
}

template <bool PUSH, bool GLOBAL, class TYPE> REFLEX_INLINE void ProgramImpl::AssignValue(ContextImpl & context, const Instruction & instruction)
{
	auto & var = GetVar<GLOBAL,TYPE>(context, instruction);

	var = Pop<TYPE,!PUSH>(context.stack);
}

template <bool PUSH, bool GLOBAL, class OBJECT> REFLEX_INLINE void ProgramImpl::AssignObject(ContextImpl & context, const Instruction & instruction)
{
	auto & var = GetVar<GLOBAL,OBJECT*>(context, instruction);

	OBJECT * & target = Pop<OBJECT*,!PUSH>(context.stack);

	Reflex::Detail::SetReferenceCountedPointer(var, target);
}

template <bool OBJECTS> REFLEX_INLINE void ProgramImpl::PushStackFrame(ContextImpl & context, const Layout & layout)
{
	auto & stack = context.stack;

	auto base = stack.GetSize();

	auto top = base + UInt(sizeof(void*));

	UInt size = layout.size;

	stack.SetSize<kAllocateNone>(top + size);

	auto pstack = stack.GetData();


	//need to store where this stackframe is, because of abort mechanism)

	auto & stackframes = context.m_stackframes;

	stackframes.Push<kAllocateNone>(pstack + top);

	auto pstackframelayout = Reinterpret<Layout*>(pstack + base);

	*pstackframelayout = RemoveConst(&layout);

	auto stackframe = pstack + top;

	InitialiseLayout<OBJECTS,false>(context, layout, stackframe);

	context.m_current_stackframe = stackframe;
}

template <bool OBJECTS, bool LOCAL> REFLEX_INLINE void ProgramImpl::PopStackFrame(ContextImpl & context, const Layout & layout)
{
	auto & stack = context.stack;

	auto & stackframes = context.m_stackframes;

	auto stackframe = stackframes.GetLast();

	DestroyLayout<OBJECTS,false>(layout, stackframe);

	stack.SetSize<kAllocateNone>(UInt((stackframe - sizeof(void*)) - stack.GetData()));

	stackframes.Pop();

	//direct access optimisation

	if (LOCAL)
	{
		REFLEX_ASSERT(stackframes);

		context.m_current_stackframe = stackframes.GetLast();
	}
}

REFLEX_INLINE void ProgramImpl::Switch(ContextImpl & context, UInt value, const Instruction * & pinstruction)
{
	auto & cases = ToPointer<SwitchTable>(Reinterpret<UIntNative>(pinstruction->param64))->cases;

	if (auto next = SearchValue<KeyCompare>(cases, value))
	{
		pinstruction = ToPointer<Instruction>(next->b);
	}
	else
	{
		pinstruction = ToPointer<Instruction>(cases.GetLast().b);
	}

	pinstruction--;
}

template <class TYPE> REFLEX_INLINE void ProgramImpl::AddAssign(ContextImpl & context)
{
	VM_POP(TYPE&,TYPE);
	args.a += args.b;
}

template <class TYPE> REFLEX_INLINE void ProgramImpl::SubtractAssign(ContextImpl & context)
{
	VM_POP(TYPE&,TYPE);
	args.a -= args.b;
}

template <class TYPE> REFLEX_INLINE void ProgramImpl::MulAssign(ContextImpl & context)
{
	VM_POP(TYPE&,TYPE);
	args.a *= args.b;
}

#define DEFINE(CODE) const Reflex::CString::View & ProgramImpl::REFLEX_CONCATENATE(kOpcode_,CODE) = InitOpcode(OPCODE(CODE), REFLEX_STRINGIFY(CODE)); REFLEX_INLINE void ProgramImpl::CODE(ContextImpl & context, const Instruction * & pinstruction)

DEFINE(PushConst)
{
	auto & instruction = *pinstruction;

	context.stack.Append<kAllocateNone>({ &Reinterpret<UInt8>(instruction.param64), Reinterpret<Pair<UInt16>>(instruction.param32).b });
}

//DEFINE(PushExternalObject)
//{
//	auto & instruction = *pinstruction;
//
//	auto & pointer = *RemoveConst(ToPointer<Object>(Reinterpret<UIntNative>(instruction.param64)));
//
//	Push(context.stack, pointer);
//}

DEFINE(PushGlobal)
{
	PushValueGeneric<true>(context, *pinstruction);
}

DEFINE(PushGlobalByAdr)
{
	Push(context.stack, &GetVar<true,UIntNative>(context, *pinstruction));
}

DEFINE(PushLocal)
{
	PushValueGeneric<false>(context, *pinstruction);
}

DEFINE(PushLocalByAdr)
{
	Push(context.stack, &GetVar<false,UIntNative>(context, *pinstruction));
}

DEFINE(PushMember)
{
	auto & instruction = *pinstruction;

	auto pobject = Pop<Pointer>(context.stack);

	auto info = Reinterpret<Pair<Int16,UInt16>>(instruction.param32);

	context.stack.Append<kAllocateNone>({ Reinterpret<UInt8>(pobject) + info.a, info.b });
}

DEFINE(PushMemberByAdr)
{
	auto pobject = Pop<UInt8*>(context.stack);

	Push(context.stack, ToUIntNative(pobject + Reinterpret<UInt16>(pinstruction->param32)));
}

//DEFINE(PushGlobalObject)
//{
//	auto pointer = GetVar<true,Pointer>(context, *pinstruction);
//
//	Push(context.stack, pointer);
//}
//
//DEFINE(PushLocalObject)
//{
//	auto pointer = GetVar<false,Pointer>(context, *pinstruction);
//
//	Push(context.stack, pointer);
//}
//
//DEFINE(PushMemberObject)
//{
//	auto & instruction = *pinstruction;
//
//	auto info = Reinterpret<Pair<Int16,UInt16>>(instruction.param32);
//
//	auto & pointer = Pop<Pointer,false>(context.stack);
//
//	pointer = reinterpret_cast<Pointer&>(Reinterpret<UInt8>(pointer)[info.a]);
//}

DEFINE(AssignGlobalValue)
{
	AssignGeneric<true>(context, *pinstruction);
}

DEFINE(AssignLocalValue)
{
	AssignGeneric<false>(context, *pinstruction);
}

DEFINE(AssignMemberValue)
{
	auto & instruction = *pinstruction;

	auto info = Reinterpret< Pair <Int16,UInt16> >(instruction.param32);

	auto bytes = Pop(context.stack, info.b);

	auto pobject = Pop<Pointer>(context.stack);

	auto pvar = Reinterpret<UInt8>(pobject) + info.a;

	MemCopy(bytes, pvar, info.b);
}

DEFINE(AssignGlobalObject)
{
	AssignObject<false,true,Object>(context, *pinstruction);
}

DEFINE(AssignLocalObject)
{
	AssignObject<false,false,Object>(context, *pinstruction);
}

DEFINE(AssignMemberObject)
{
	auto & instruction = *pinstruction;

	VM_POP(Pointer, Pointer);

	auto & member = reinterpret_cast<Pointer&>(Reinterpret<UInt8>(args.a)[Reinterpret<UInt16>(instruction.param32)]);

	Reflex::Detail::SetReferenceCountedPointer(member, args.b);
}

DEFINE(Discard)
{
	context.stack.Shrink(pinstruction->param32);
}

DEFINE(SwizzleTemporaryValue)
{
	auto & stack = context.stack;

	auto type = Reinterpret<TypeRef>(pinstruction->param64);

	auto info = Reinterpret<Pair<UInt16>>(pinstruction->param32);

	//auto top = info.a + info.b;

	//auto discard = type->size - top;

	context.workspace = { Pop(stack, type->size), type->size };

	stack.Append({ context.workspace.GetData() + info.a, info.b});
}

DEFINE(Jump)
{
	pinstruction = ToPointer<Instruction>(UIntNative(pinstruction->param64));

	pinstruction--;
}

DEFINE(JumpIfTrue8)
{
	if (Pop<UInt8>(context.stack)) Jump(context, pinstruction);
}

DEFINE(JumpIfFalse8)
{
	if (!Pop<UInt8>(context.stack)) Jump(context, pinstruction);
}

DEFINE(JumpIfTrue32)
{
	if (Pop<Value32>(context.stack)) Jump(context, pinstruction);
}

DEFINE(JumpIfFalse32)
{
	if (!Pop<Value32>(context.stack)) Jump(context, pinstruction);
}

DEFINE(Switch8)
{
	Switch(context, Pop<UInt8>(context.stack), pinstruction);
}

DEFINE(Switch32)
{
	Switch(context, Pop<UInt32>(context.stack), pinstruction);
}

DEFINE(CallExternal)
{
	auto fnptr = Reinterpret<ExternalFunctionPtr>(pinstruction->param64);

	(*fnptr)(context);
}

DEFINE(BindFnObject)
{
	auto MakeCaptures = [](Pair <UInt16> n, const Instruction * & pos, Stack & stack)
	{
		stack.Shrink(n.b);

		auto pcaptures = stack.GetData() + stack.GetSize();

		Pair <UInt,LayoutTemplate> captures;

		captures.a = n.a;

		auto & layout = captures.b;

		layout.init_state.Allocate(n.b);

		REFLEX_LOOP_PTR(pos, itr, n.a)
		{
			REFLEX_ASSERT(itr->opcode == OPCODE(Data));

			auto & type = *ToPointer<Type>(Reinterpret<UIntNative>(itr->param64));

			if (type.IsObject())
			{
				auto ref = *reinterpret_cast<Object**>(pcaptures);

				layout.AddObject<false>(type, "", ref);
			}
			else
			{
				layout.AddValue(type, "", { pcaptures, type.size });
			}

			pcaptures += type.size;
		}

		pos += n.a;

		pos--;	//!do not merge with above line, results in UInt semantic

		return captures;
	};

	auto & pos = context.instructionptr;

	auto & instruction = *pos++;

	auto & data = *pos++;

	REFLEX_ASSERT(data.opcode == OPCODE(Data));

	auto & fn = *ToPointer<Function>(Reinterpret<UIntNative>(instruction.param64));

	auto & fnobject_t = *ToPointer<Type>(Reinterpret<UIntNative>(data.param64));

	auto size = Reinterpret< Pair<UInt16> >(data.param32);

	auto fnobject = FnObjectImpl::Create(context, Cast<FnObjectImpl::Type>(fnobject_t), ContainerType(instruction.param32), fn, MakeCaptures(size, pos, context.stack));

	VM_RTN(fnobject);
}

template <bool OBJECTS> REFLEX_INLINE void ProgramImpl::CallFnImpl(ContextImpl & context, const Instruction * & current)
{
	auto & fn = *ToPointer<ScriptFunction>(Reinterpret<UIntNative>(current->param64));

	Push(context.stack, CurrentPositionStore({ current,0,UInt16(context.stack.GetSize()) }));	//stack size is DEBUG INFO

	PushStackFrame<OBJECTS>(context, *fn.layout);

	current = fn.instructions.GetData() - 1;
}

DEFINE(CallFn)
{
	CallFnImpl<true>(context, pinstruction);
}

DEFINE(CallFnObject)
{
	auto & stack = context.stack;

	auto & fnobject = **Reinterpret<FnObjectImpl*>(stack.GetData() + stack.GetSize() - pinstruction->param64);

	fnobject.CallInternal(context);
}

template <class RETURN_HANDLER> REFLEX_INLINE void ProgramImpl::ReturnImpl(ContextImpl & context, const Instruction * & pinstruction)
{
	auto & stack = context.stack;

	auto & fn = *ToPointer<ScriptFunction>(UIntNative(pinstruction->param64));

	RETURN_HANDLER rtnhandler(context, fn);

	PopStackFrame<true>(context, *fn.layout);

	auto caller = Pop<CurrentPositionStore>(stack);

	pinstruction = caller.a;

	auto size = caller.b;

	size += fn.arguments_size;

	stack.Shrink(size);

	rtnhandler.Apply(context);
}

DEFINE(Return)
{
	ReturnImpl<ValueReturnHandler>(context, pinstruction);
}

DEFINE(ReturnObject)
{
	ReturnImpl< ObjectReturnHandler <Object> >(context, pinstruction);
}

DEFINE(Marker) { /*REFLEX_ASSERT(false);*/ }

DEFINE(Data) 
{
#if REFLEX_DEBUG
	REFLEX_ASSERT(!(pinstruction->param64 | pinstruction->param32));	//only allowed if used as noop, otherwise should be read by previous instruction
#endif
}

//intrinsics

DEFINE(intrinsicNullObject)
{
	VM_RTN(VM::Detail::GetNull(context, ToPointer<Type>(UIntNative(pinstruction->param64))));
}

DEFINE(intrinsicNewObject)
{
	auto object = CreateObject(context, ToPointer<Type>(UIntNative(pinstruction->param64)));

	REFLEX_ASSERT(object->GetContextID() == context.GetContextID());

	VM_RTN(object);
}

DEFINE(intrinsicObjectCast)
{
	auto & pointer = Pop<Pointer,false>(context.stack);

	auto classinfo = ToPointer<Reflex::Detail::DynamicTypeInfo>(UIntNative(pinstruction->param64));

	if (!Reflex::Detail::CheckObjectType(*pointer, classinfo))
	{
		pointer = context.GetNullByRTTID(classinfo->type_id, 0);
	}
}

DEFINE(intrinsicValueEqual)
{
	auto size = UInt(pinstruction->param64);

	auto b = Pop(context.stack, size);
	auto a = Pop(context.stack, size);

	context.stack.Push(MemCompare(a, b, size));
}

DEFINE(intrinsicValueInequal)
{
	auto size = UInt(pinstruction->param64);

	auto b = Pop(context.stack, size);
	auto a = Pop(context.stack, size);

	context.stack.Push(!MemCompare(a, b, size));
}

DEFINE(intrinsicLogicalNot8)
{
	context.stack.Push(!Pop<bool>(context.stack));
}

DEFINE(intrinsicLogicalNot32)
{
	context.stack.Push(!Pop<UInt32>(context.stack));
}

DEFINE(intrinsicLogicalOr8)
{
	VM_POP(UInt8, UInt8);

	context.stack.Push(args.a || args.b);
}

DEFINE(intrinsicLogicalOr32)
{
	VM_POP(UInt32,UInt32);

	context.stack.Push(args.a || args.b);
}

DEFINE(intrinsicLogicalAnd8)
{
	VM_POP(UInt8,UInt8);

	context.stack.Push(args.a && args.b);
}

DEFINE(intrinsicLogicalAnd32)
{
	VM_POP(UInt32,UInt32);

	context.stack.Push(args.a && args.b);
}

DEFINE(intrinsicInt32LessThan)
{
	VM_POP(Int32,Int32);
	VM_RTN(UInt8(args.a < args.b));
}

DEFINE(intrinsicInt32GreaterThan)
{
	VM_POP(Int32,Int32);
	VM_RTN(UInt8(args.a > args.b));
}

DEFINE(intrinsicPreIncInt32)
{
	VM_RTN(++Pop<Int32&>(context.stack));
}

DEFINE(intrinsicPreDecInt32)
{
	VM_RTN(--Pop<Int32&>(context.stack));
}

DEFINE(intrinsicPostIncInt32)
{
	auto & value = Pop<Int32&>(context.stack);

	Push(context.stack, value++);
}

DEFINE(intrinsicPostDecInt32)
{
	auto & value = Pop<Int32&>(context.stack);

	Push(context.stack, value--);
}

DEFINE(intrinsicAddAssignInt32) { AddAssign<Int32>(context); }
DEFINE(intrinsicSubtractAssignInt32) { SubtractAssign<Int32>(context); }
DEFINE(intrinsicMulAssignInt32) { MulAssign<Int32>(context); }

DEFINE(intrinsicInvertInt32)
{
	auto & value = Pop<Int32,false>(context.stack);

	value = -value;
}

DEFINE(intrinsicAddInt32Pair)
{
	VM_POP(Int32,Int32);
	VM_RTN(args.a + args.b);
}

DEFINE(intrinsicSubInt32Pair)
{
	VM_POP(Int32,Int32);
	VM_RTN(args.a - args.b);
}

DEFINE(intrinsicMulInt32Pair)
{
	VM_POP(Int32,Int32);
	VM_RTN(args.a * args.b);
}

DEFINE(intrinsicDivInt32Pair)
{
	VM_POP(Int32,Int32);
	VM_RTN(args.b ? (args.a / args.b) : 0);
}

DEFINE(intrinsicFloat32LessThan)
{
	VM_POP(Float32,Float32);
	VM_RTN(UInt8(args.a < args.b));
}

DEFINE(intrinsicFloat32GreaterThan)
{
	VM_POP(Float32, Float32);
	VM_RTN(UInt8(args.a > args.b));
}

DEFINE(intrinsicInt32ToFloat32)
{
	auto & value = Pop<Int32,false>(context.stack);

	Reinterpret<Float32>(value) = ToFloat32(value);
}

DEFINE(intrinsicInvertFloat32)
{
	auto & value = Pop<Float32,false>(context.stack);

	value = -value;
}

DEFINE(intrinsicAddFloat32Pair)
{
	VM_POP(Float32, Float32);
	VM_RTN(args.a + args.b);
}

DEFINE(intrinsicSubFloat32Pair)
{
	VM_POP(Float32, Float32);
	VM_RTN(args.a - args.b);
}

DEFINE(intrinsicMulFloat32Pair)
{
	VM_POP(Float32, Float32);
	VM_RTN(args.a * args.b);
}

DEFINE(intrinsicDivFloat32Pair)
{
	VM_POP(Float32, Float32);
	VM_RTN(args.a / (args.b ? args.b : 1.0f));
}

DEFINE(intrinsicAddAssignFloat32) { AddAssign<Float32>(context); }
DEFINE(intrinsicSubtractAssignFloat32) { SubtractAssign<Float32>(context); }
DEFINE(intrinsicMulAssignFloat32) { MulAssign<Float32>(context); }

DEFINE(intrinsicInt32Next)
{
	VM_POP(Int32Iterator&,Int32&);

	auto & itr = args.a;

	if (itr.m_value < itr.m_n)
	{
		args.b = itr.m_value++;

		VM_RTN(true);
	}
	else
	{
		VM_RTN(false);
	}
}

template <class TYPE> REFLEX_INLINE void ProgramImpl::optimisedIntegralNext(ContextImpl & context)
{
	typedef IntegralArrayIterator Iterator;

	VM_POP(Iterator&,TYPE&);

	Push(context.stack, args.a.template optimisedNext<TYPE>(args.b));
}

DEFINE(intrinsicIntegralArrayNext)
{
	typedef IntegralArrayIterator Iterator;

	VM_POP(Iterator&,UInt8&);

	VM_RTN(args.a.Next(args.b, Reinterpret<UIntNative>(pinstruction->param64)));
}

DEFINE(intrinsicObjectArrayNext)
{
	typedef ObjectArrayIterator Iterator;

	VM_POP(Iterator&,Object*&);

	VM_RTN(args.a.Next<Object>(args.b));
}

DEFINE(intrinsicValueArraySet)
{
	auto & itemsize = Reinterpret<Pair<UInt16>>(pinstruction->param32);

	auto value = Pop(context.stack, itemsize.b);

	VM_POP(ValueArray&,Int32);

	auto & array = args.a;

	auto offset = (args.b % array.m_wrap) * itemsize.b;

	MemCopy(value, Reinterpret<UInt8>(array.m_ptr) + offset, itemsize.b);
}

DEFINE(intrinsicValueArrayGet)
{
	auto & itemsize = Reinterpret<Pair<UInt16>>(pinstruction->param32);

	VM_POP(ValueArray&,Int32);

	auto & array = args.a;

	auto offset = (args.b % array.m_wrap) * itemsize.b;

	context.stack.Append({ Reinterpret<UInt8>(array.m_ptr) + offset, itemsize.b});
}

DEFINE(intrinsicValueArray32Set)
{
	VM_POP(ValueArray&,Int32,Value32);

	auto & array = args.a;

	Reinterpret<Value32>(array.m_ptr)[(args.b % array.m_wrap)] = args.c;
}

DEFINE(intrinsicValueArray32Get)
{
	VM_POP(ValueArray&,Int32);

	auto & array = args.a;

	VM_RTN(Reinterpret<Value32>(array.m_ptr)[args.b % array.m_wrap]);
}

DEFINE(intrinsicStringToBool)
{
	VM_RTN(bool(Pop<String&>(context.stack).size));
}

DEFINE(intrinsicStringToKey32)
{
	auto & string = Pop<String&>(context.stack);

	VM_RTN(string.hash);
}

DEFINE(intrinsicStringEqual)
{
	VM_POP(String*,String*);

	VM_RTN(UInt8(args.a->GetView() == args.b->GetView()));
}

DEFINE(intrinsicStringInequal)
{
	VM_POP(String*,String*);

	VM_RTN(UInt8(args.a->GetView() != args.b->GetView()));
}

DEFINE(intrinsicObjectMethod_Void)
{
	auto & fn = *reinterpret_cast< ObjectMethodPointerType_0P <void>* >(pinstruction->param64);

	VM_POP1(Object&);

	(arg.*fn)();
}

DEFINE(intrinsicObjectMethod_Value8)
{
	auto & fn = *reinterpret_cast< ObjectMethodPointerType_0P <UInt8>* >(pinstruction->param64);

	VM_POP1(Object&);

	VM_RTN((arg.*fn)());
}

DEFINE(intrinsicObjectMethod_Value32)
{
	auto & fn = *reinterpret_cast< ObjectMethodPointerType_0P <UInt32>* >(pinstruction->param64);

	VM_POP1(Object&);

	VM_RTN((arg.*fn)());
}

DEFINE(intrinsicObjectMethod_Float32)
{
	auto & fn = *reinterpret_cast< ObjectMethodPointerType_0P <Float32>* >(pinstruction->param64);

	VM_POP1(Object&);

	VM_RTN((arg.*fn)());
}

DEFINE(intrinsicObjectMethod_Object)
{
	auto & fn = *reinterpret_cast< ObjectMethodPointerType_0P <TRef<Object>>* >(pinstruction->param64);

	VM_POP1(Object&);

	VM_RTN((arg.*fn)());
}

DEFINE(intrinsicObjectMethod_Void_Object)
{
	auto & fn = *reinterpret_cast< ObjectMethodPointerType_1P <void,Pointer>* >(pinstruction->param64);

	VM_POP(Object&,Object*);

	(args.a.*fn)(args.b);
}

DEFINE(intrinsicObjectMethod_Void_Value8)
{
	auto & fn = *reinterpret_cast< ObjectMethodPointerType_1P <void,UInt8>* >(pinstruction->param64);

	VM_POP(Object&,UInt8);

	(args.a.*fn)(args.b);
}

DEFINE(intrinsicObjectMethod_Void_Value32)
{
	auto & fn = *reinterpret_cast< ObjectMethodPointerType_1P <void,UInt32>* >(pinstruction->param64);

	VM_POP(Object&, UInt32);

	(args.a.*fn)(args.b);
}

DEFINE(intrinsicObjectMethod_Void_Float32)
{
	auto & fn = *reinterpret_cast< ObjectMethodPointerType_1P <void,Float32>* >(pinstruction->param64);

	VM_POP(Object&,Float32);

	(args.a.*fn)(args.b);
}

DEFINE(intrinsicObjectMethod_Void_Value32_Value32)
{
	auto & fn = *reinterpret_cast< ObjectMethodPointerType_2P <void,UInt32,UInt32>* >(pinstruction->param64);

	VM_POP(Object&,UInt32,UInt32);

	(args.a.*fn)(args.b, args.c);
}

DEFINE(intrinsicObjectMethod_Void_Float32_Float32)
{
	auto & fn = *reinterpret_cast< ObjectMethodPointerType_2P <void,Float32,Float32>* >(pinstruction->param64);

	VM_POP(Object&,Float32, Float32);

	(args.a.*fn)(args.b, args.c);
}

DEFINE(intrinsicObjectMethod_Float32_Value32)
{
	auto & fn = *reinterpret_cast< ObjectMethodPointerType_1P <Float32,UInt32>* >(pinstruction->param64);

	VM_POP(Object&,UInt32);

	VM_RTN((args.a.*fn)(args.b));
}

DEFINE(intrinsicObjectMethod_Value8_Object)
{
	auto & fn = *reinterpret_cast< ObjectMethodPointerType_1P <UInt8,Pointer>* >(pinstruction->param64);

	VM_POP(Object&,Pointer);

	VM_RTN((args.a.*fn)(args.b));
}

DEFINE(intrinsicObjectMethod_Value8_Value32)
{
	auto & fn = *reinterpret_cast< ObjectMethodPointerType_1P <UInt8,UInt32>* >(pinstruction->param64);

	VM_POP(Object&,UInt32);

	VM_RTN((args.a.*fn)(args.b));
}

DEFINE(intrinsicObjectMethod_Value32_Object)
{
	auto & fn = *reinterpret_cast< ObjectMethodPointerType_1P <UInt32,Pointer>* >(pinstruction->param64);

	VM_POP(Object&,Pointer);

	VM_RTN((args.a.*fn)(args.b));
}

DEFINE(intrinsicObjectMethod_Object_Value32)
{
	auto & fn = *reinterpret_cast< ObjectMethodPointerType_1P <TRef<Object>,UInt32>* >(pinstruction->param64);

	VM_POP(Object&,UInt32);

	VM_RTN((args.a.*fn)(args.b).Adr());
}

DEFINE(intrinsicObjectMethod_Void_Value32_Object)
{
	auto & fn = *reinterpret_cast< ObjectMethodPointerType_2P <void,UInt32,Pointer>* >(pinstruction->param64);

	VM_POP(Object&,UInt32,Pointer);

	(args.a.*fn)(args.b, args.c);
}

DEFINE(intrinsicObjectMethod_Object_Value32_Value32)
{
	auto & fn = *reinterpret_cast< ObjectMethodPointerType_2P <TRef<Object>,UInt32,UInt32>* >(pinstruction->param64);

	VM_POP(Object&,UInt32,UInt32);

	VM_RTN((args.a.*fn)(args.b, args.c));
}

DEFINE(intrinsicObjectMethod_Value8_Object_Object)
{
	auto & fn = *reinterpret_cast< ObjectMethodPointerType_2P <UInt8,Pointer,Pointer>* >(pinstruction->param64);

	VM_POP(Object&,Pointer,Pointer);

	VM_RTN((args.a.*fn)(args.b, args.c));
}

DEFINE(optimisationPushConst8)
{
	VM_RTN(Reinterpret<Value8>(pinstruction->param64));
}

DEFINE(optimisationPushConst32)
{
	VM_RTN(Reinterpret<Value32>(pinstruction->param64));
}

DEFINE(optimisationPushConst64)
{
	VM_RTN(pinstruction->param64);
}

DEFINE(optimisationSwitch8JumpTable)
{
	auto jumptable = ToPointer<SwitchTable>(Reinterpret<UIntNative>(pinstruction->param64))->cases;

	auto value = Pop<UInt8>(context.stack);

	if (value < 16)
	{
		pinstruction = ToPointer<Instruction>(jumptable[value].b);
	}
	else
	{
		pinstruction = ToPointer<Instruction>(jumptable[16].b);
	}

	pinstruction--;
}

DEFINE(optimisationValueEqual32)
{
	VM_POP(Value32,Value32);
	VM_RTN(UInt8(args.a == args.b));
}

DEFINE(optimisationValueInequal32)
{
	VM_POP(Value32,Value32);
	VM_RTN(UInt8(args.a != args.b));
}

DEFINE(optimisationValueEqual64)
{
	VM_POP(Value64,Value64);
	VM_RTN(UInt8(args.a == args.b));
}

DEFINE(optimisationValueInequal64)
{
	VM_POP(Value64,Value64);
	VM_RTN(UInt8(args.a != args.b));
}

DEFINE(optimisationPushGlobal8)
{
	PushValue<true,UInt8>(context, *pinstruction);
}

DEFINE(optimisationPushGlobal32)
{
	PushValue<true,Value32>(context, *pinstruction);
}

DEFINE(optimisationPushGlobal64)
{
	PushValue<true,Value64>(context, *pinstruction);
}

DEFINE(optimisationPushGlobal32Pair)
{
	PushValuePair<true,Value32>(context, *pinstruction);
}

DEFINE(optimisationPushGlobal64Pair)
{
	PushValuePair<true,Value64>(context, *pinstruction);
}

DEFINE(optimisationPushGlobal64and32)
{
	PushValue64and32<true>(context, *pinstruction);
}

DEFINE(optimisationPushGlobal32andConst32)
{
	PushValueandConst32<true,Value32>(context, *pinstruction);
}

DEFINE(optimisationPushGlobal64andConst32)
{
	PushValueandConst32<true,Value64>(context, *pinstruction);
}

DEFINE(optimisationPushLocal8)
{
	PushValue<false,UInt8>(context, *pinstruction);
}

DEFINE(optimisationPushLocal32)
{
	PushValue<false,Value32>(context, *pinstruction);
}

DEFINE(optimisationPushLocal64)
{
	PushValue<false,Value64>(context, *pinstruction);
}

DEFINE(optimisationPushLocal32Pair)
{
	PushValuePair<false,Value32>(context, *pinstruction);
}

DEFINE(optimisationPushLocal64Pair)
{
	PushValuePair<false,Value64>(context, *pinstruction);
}

DEFINE(optimisationPushLocal64and32)
{
	PushValue64and32<false>(context, *pinstruction);
}

DEFINE(optimisationPushLocal32andConst32)
{
	PushValueandConst32<false,Value32>(context, *pinstruction);
}

DEFINE(optimisationPushLocal64andConst32)
{
	PushValueandConst32<false,Value64>(context, *pinstruction);
}

DEFINE(optimisationPushMember8)
{
	auto pmember = Pop<UInt8*>(context.stack) + Reinterpret<UInt16>(pinstruction->param32);

	VM_RTN(*pmember);
}

DEFINE(optimisationPushMember32)
{
	auto pmember = Pop<UInt8*>(context.stack) + Reinterpret<UInt16>(pinstruction->param32);

	VM_RTN(*Reinterpret<Value32>(pmember));
}

DEFINE(optimisationPushMember64)
{
	auto pmember = Pop<UInt8*>(context.stack) + Reinterpret<UInt16>(pinstruction->param32);

	VM_RTN(*Reinterpret<Value64>(pmember));
}

DEFINE(optimisationAssignGlobalValue8)
{
	AssignValue<false,true,Value8>(context, *pinstruction);
}

DEFINE(optimisationAssignGlobalValue32)
{
	AssignValue<false,true,Value32>(context, *pinstruction);
}

DEFINE(optimisationAssignGlobalValue64)
{
	AssignValue<false,true,Value64>(context, *pinstruction);
}

DEFINE(optimisationAssignGlobalObjectST)
{
	AssignObject<false,true,ObjectST>(context, *pinstruction);
}

DEFINE(optimisationAssignLocalValue8)
{
	AssignValue<false,false,Value8>(context, *pinstruction);
}

DEFINE(optimisationAssignLocalValue32)
{
	AssignValue<false,false,Value32>(context, *pinstruction);
}

DEFINE(optimisationAssignLocalValue64)
{
	AssignValue<false,false,Value64>(context, *pinstruction);
}

DEFINE(optimisationAssignLocalObjectST)
{
	AssignObject<false,false,ObjectST>(context, *pinstruction);
}

DEFINE(optimisationAssignMemberValue8)
{
	VM_POP(UInt8*, Value8);

	*Reinterpret<Value8>(args.a + Reinterpret<UInt16>(pinstruction->param32)) = args.b;
}

DEFINE(optimisationAssignMemberValue32)
{
	VM_POP(UInt8*,Value32);

	*Reinterpret<Value32>(args.a + Reinterpret<UInt16>(pinstruction->param32)) = args.b;
}

DEFINE(optimisationAssignMemberValue64)
{
	VM_POP(UInt8*, Value64);

	*Reinterpret<Value64>(args.a + Reinterpret<UInt16>(pinstruction->param32)) = args.b;
}

DEFINE(optimisationAssignMemberObjectST)
{
	auto & instruction = *pinstruction;

	VM_POP(Pointer,PointerST);

	auto & member = reinterpret_cast<PointerST&>(Reinterpret<UInt8>(args.a)[Reinterpret<UInt16>(instruction.param32)]);

	Reflex::Detail::SetReferenceCountedPointer(member, args.b);
}

DEFINE(optimisationAssignPushLocalObject)
{
	AssignObject<true,false,Object>(context, *pinstruction);
}

DEFINE(optimisationAssignPushLocalObjectST)
{
	AssignObject<true,false,ObjectST>(context, *pinstruction);
}

DEFINE(optimisationAssignPushGlobalObject)
{
	AssignObject<true,true,Object>(context, *pinstruction);
}

DEFINE(optimisationAssignPushGlobalObjectST)
{
	AssignObject<true,true,ObjectST>(context, *pinstruction);
}

//DEFINE(optimisationAssignPushMemberObject)
//{
//	auto & instruction = *pinstruction;
//
//	VM_POP(Pointer, Pointer);
//
//	auto & member = reinterpret_cast<Pointer&>(Reinterpret<UInt8>(args.a)[Reinterpret<UInt16>(instruction.param32)]);
//
//	Reflex::Detail::SetReferenceCountedPointer(member, args.b);
//
//	Push(context.stack, member);
//}
//
//DEFINE(optimisationAssignPushMemberObjectST)
//{
//	auto & instruction = *pinstruction;
//
//	VM_POP(Pointer, PointerST);
//
//	auto & member = reinterpret_cast<PointerST&>(Reinterpret<UInt8>(args.a)[Reinterpret<UInt16>(instruction.param32)]);
//
//	Reflex::Detail::SetReferenceCountedPointer(member, args.b);
//
//	Push(context.stack, member);
//}

DEFINE(optimisationDiscard8)
{
	context.stack.Pop();
}

DEFINE(optimisationDiscard32)
{
	context.stack.Shrink(4);
}

DEFINE(optimisationDiscard64)
{
	context.stack.Shrink(8);
}

DEFINE(optimisationCallFnOfValues)
{
	CallFnImpl<false>(context, pinstruction);
}

DEFINE(optimisationReturnVoid)
{
	ReturnImpl<VoidReturnHandler>(context, pinstruction);
}

DEFINE(optimisationReturn8)
{
	ReturnImpl<SizedReturnHandler<Int8>>(context, pinstruction);
}

DEFINE(optimisationReturn32)
{
	ReturnImpl<SizedReturnHandler<Int32>>(context, pinstruction);
}

DEFINE(optimisationReturn64)
{
	ReturnImpl<SizedReturnHandler<Value64>>(context, pinstruction);
}

DEFINE(optimisationReturnObjectST)
{
	ReturnImpl< ObjectReturnHandler <ObjectST> >(context, pinstruction);
}

DEFINE(optimisationValueArray32Next)
{
	optimisedIntegralNext<Value32>(context);
}

DEFINE(optimisationValueArray64Next)
{
	optimisedIntegralNext<Value64>(context);
}

DEFINE(optimisationObjectArrayNextST)
{
	typedef ObjectArrayIterator Iterator;

	VM_POP(Iterator&,ObjectST*&);

	Push(context.stack, args.a.Next<ObjectST>(args.b));
}

REFLEX_END_INTERNAL




//
//

Reflex::TRef <Reflex::VM::Program> Reflex::VM::Program::Create(const Bindings & bindings)
{
	return REFLEX_CREATE(Detail::ProgramImpl, bindings);
}

Reflex::VM::Program::Program(const Bindings & bindings)
	: bindings(RemoveConst(bindings))
{
}

#include "programimpl.h"





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

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

struct Optimiser
{
	inline static const CString::View kArgNames[16] = { "t00", "t01", "t02", "t03", "t04", "t05", "t06", "t07", "t08", "t09", "t10", "t11", "t12", "t13", "t14", "t15" };

	struct Global
	{
		static inline const auto kPush8 = OPCODE(optimisationPushGlobal8);
		static inline const auto kPush32 = OPCODE(optimisationPushGlobal32);
		static inline const auto kPush64 = OPCODE(optimisationPushGlobal64);

		static inline const auto kPush32Pair = OPCODE(optimisationPushGlobal32Pair);
		static inline const auto kPush64Pair = OPCODE(optimisationPushGlobal64Pair);
		static inline const auto kPush64and32 = OPCODE(optimisationPushGlobal64and32);

		static inline const auto kPush32withConst32 = OPCODE(optimisationPushGlobal32andConst32);
		static inline const auto kPush64withConst32 = OPCODE(optimisationPushGlobal64andConst32);

		static inline const auto kAssignValue8 = OPCODE(optimisationAssignGlobalValue8);
		static inline const auto kAssignValue32 = OPCODE(optimisationAssignGlobalValue32);
		static inline const auto kAssignValue64 = OPCODE(optimisationAssignGlobalValue64);
		static inline const auto kAssignObjectST = OPCODE(optimisationAssignGlobalObjectST);

		//static inline const auto kAssignPushValue8 = OPCODE(optimisationAssignPushGlobalValue8);
		//static inline const auto kAssignPushValue32 = OPCODE(optimisationAssignPushGlobalValue32);
		//static inline const auto kAssignPushValue64 = OPCODE(optimisationAssignPushGlobalValue64);

		static inline const auto kPushObjectOpcode = sizeof(Pointer) == 8 ? OPCODE(optimisationPushGlobal64) : OPCODE(optimisationPushGlobal32);

		static inline const auto kAssignPushObject = OPCODE(optimisationAssignPushGlobalObject);
		static inline const auto kAssignPushObjectST = OPCODE(optimisationAssignPushGlobalObjectST);
	};

	struct Local
	{
		static inline const auto kPush8 = OPCODE(optimisationPushLocal8);
		static inline const auto kPush32 = OPCODE(optimisationPushLocal32);
		static inline const auto kPush64 = OPCODE(optimisationPushLocal64);

		static inline const auto kPush32Pair = OPCODE(optimisationPushLocal32Pair);
		static inline const auto kPush64Pair = OPCODE(optimisationPushLocal64Pair);
		static inline const auto kPush64and32 = OPCODE(optimisationPushLocal64and32);

		static inline const auto kPush32withConst32 = OPCODE(optimisationPushLocal32andConst32);
		static inline const auto kPush64withConst32 = OPCODE(optimisationPushLocal64andConst32);

		static inline const auto kAssignValue8 = OPCODE(optimisationAssignLocalValue8);
		static inline const auto kAssignValue32 = OPCODE(optimisationAssignLocalValue32);
		static inline const auto kAssignValue64 = OPCODE(optimisationAssignLocalValue64);
		static inline const auto kAssignObjectST = OPCODE(optimisationAssignLocalObjectST);

		//static inline const auto kAssignPushValue8 = OPCODE(optimisationAssignPushLocalValue8);
		//static inline const auto kAssignPushValue32 = OPCODE(optimisationAssignPushLocalValue32);
		//static inline const auto kAssignPushValue64 = OPCODE(optimisationAssignPushLocalValue64);

		static inline const auto kPushObjectOpcode = sizeof(Pointer) == 8 ? OPCODE(optimisationPushLocal64) : OPCODE(optimisationPushLocal32);

		static inline const auto kAssignPushObject = OPCODE(optimisationAssignPushLocalObject);
		static inline const auto kAssignPushObjectST = OPCODE(optimisationAssignPushLocalObjectST);
	};

	struct Member
	{
		static inline const auto kPush8 = OPCODE(optimisationPushMember8);
		static inline const auto kPush32 = OPCODE(optimisationPushMember32);
		static inline const auto kPush64 = OPCODE(optimisationPushMember64);

		static inline const auto kAssignValue8 = OPCODE(optimisationAssignMemberValue8);
		static inline const auto kAssignValue32 = OPCODE(optimisationAssignMemberValue32);
		static inline const auto kAssignValue64 = OPCODE(optimisationAssignMemberValue64);
		static inline const auto kAssignObjectST = OPCODE(optimisationAssignMemberObjectST);

		//static inline const auto kAssignPushValue8 = OPCODE(optimisationAssignPushMemberValue8);
		//static inline const auto kAssignPushValue32 = OPCODE(optimisationAssignPushMemberValue32);
		//static inline const auto kAssignPushValue64 = OPCODE(optimisationAssignPushMemberValue64);

		//static inline const auto kPushObjectOpcode = sizeof(Pointer) == 8 ? OPCODE(optimisationPushMember64) : OPCODE(optimisationPushMember32);

		//static inline const auto kAssignPushObject = OPCODE(optimisationAssignPushMemberObject);
		//static inline const auto kAssignPushObjectST = OPCODE(optimisationAssignPushMemberObjectST);
	};

	static void RemoveNext(Instructions & instructions, UInt idx);

	static void RemoveDiscards(Instructions & instructions);

	static void InlineFunctionCalls(bool local, LayoutTemplate & layout, Instructions & instructions);

	static void ApplyLocalCopyEllisions(const Compiler::State & state, LayoutTemplate & layout, Instructions & instructions);

	template <class SCOPE> static void ApplyRest_Push(Instructions & instructions, UInt idx, Instruction & i, const Instruction & next);

	template <class SCOPE> static void ApplyRest_AssignValue(Instructions & instructions, UInt idx, Instruction & i, const Instruction & next);
		
	template <class SCOPE> static void ApplyRest_AssignObject(Instructions & instructions, UInt idx, Instruction & i, const Instruction & next);

	static void ApplyRest(const Bindings & bindings, Array <Instruction> &instructions);

};

void Optimiser::RemoveNext(Instructions & instructions, UInt idx)
{
	instructions.Remove(idx + 1);
}

void Optimiser::RemoveDiscards(Instructions & instructions)
{
	REFLEX_RLOOP(idx, instructions.GetSize())
	{
		auto & i = instructions[idx];

		if (i.opcode == OPCODE(Discard))
		{
			auto & prev = (&i)[-1];

			//auto var = GetVar(i);

			auto prev_var = GetVar(prev);

			switch (prev.opcode)
			{
			case OPCODE(PushGlobal):
			case OPCODE(PushLocal):
			case OPCODE(PushMember):
				if (i.param32 == prev_var.b)
				{
					instructions.Remove(--idx, 2);

					continue;
				}
				break;

			default:
				break;
			}

			if (i.param32 == sizeof(UInt32))
			{
				i.opcode = OPCODE(optimisationDiscard32);
			}
			else if (i.param32 == sizeof(UInt8))
			{
				i.opcode = OPCODE(optimisationDiscard8);
			}
			else if (i.param32 == sizeof(UInt64))
			{
				i.opcode = OPCODE(optimisationDiscard64);
			}
		}
	}
}

void Optimiser::ApplyLocalCopyEllisions(const Compiler::State & state, LayoutTemplate & layout, Instructions & instructions)
{
	LOCAL(bool,UsedOnce)(const Instructions & instructions, UInt16 adr) //adr + size
	{
		UInt npush = 0;
		UInt nassign = 0;

		for (auto & i : instructions)
		{
			if (GetVar(i).a == adr)
			{
				if (i.opcode == OPCODE(PushLocal) || i.opcode == OPCODE(PushLocalByAdr))
				{
					if (npush++) return false;
				}
				else if (i.opcode == OPCODE(AssignLocalValue) || i.opcode == OPCODE(AssignLocalObject))
				{
					if (nassign++) return false;
				}
			}
		}

		return (npush | nassign) == 1;
	}
	END

	/*
	* full logic maybe (including non adjacent cases)
	* if VAR is written to only once
	* and its SRC is written to only once, and only before VAR is written to, and only pushed once
	* then remove VAR
	* and replace all pushes of VAR with pushes of SRC
	*/

	/*
	simpler approximate version for adjacent cases caused by compiler
	foreach (pushlocal src)
		if next = assignlocal dest and nextnext = pushlocal dest
			if dest is only pushed and assigned once
				then do illision of dest
	*/

	//auto RemoveVariable = [](LayoutTemplate & layout, UInt16 var, Array <Instruction> & instructions, UInt idx)
	//{
	//	instructions.Remove(idx, 2);
	//};

	REFLEX_MARKER(Start);

	for (auto & i : ReverseSplice<true>(instructions, 2).a)
	{
		if (i.opcode == OPCODE(AssignLocalValue) || i.opcode == OPCODE(AssignLocalObject))
		{
			auto next1 = (&i)[1];
			auto next2 = (&i)[2];

			if (next1.opcode == OPCODE(PushLocal) && i.param32 == next1.param32)
			{
				auto var = GetVar(i).a;

				if (UsedOnce::Call(instructions, var) && UsedOnce::Call(instructions, GetVar(next1).a))
				{
					if (next2.opcode == OPCODE(AssignLocalValue) || next2.opcode == OPCODE(AssignLocalObject))
					{
						//TODO remove from layout

						instructions.Remove(UInt(&i - instructions.GetData()), 2);

						//RemoveVariable(layout, var, instructions, UInt(&i - instructions.GetData()));

						goto Start;
					}
				}
			}
		}
	}
}

template <class SCOPE> void Optimiser::ApplyRest_Push(Instructions & instructions, UInt idx, Instruction & i, const Instruction & next)
{
	switch (GetVar(i).b)
	{
	case sizeof(Value8):
		i.opcode = SCOPE::kPush8;
		break;

	case sizeof(Value32):
		if (next.opcode == SCOPE::kPush32)
		{
			i.opcode = SCOPE::kPush32Pair;

			i.param32 = Reinterpret<UInt32>(MakeTuple(Reinterpret<Int16>(i.param32), Reinterpret<Int16>(next.param32)));

			RemoveNext(instructions, idx);
		}
		else if (next.opcode == OPCODE(optimisationPushConst32))
		{
			i.opcode = SCOPE::kPush32withConst32;

			i.param64 = next.param64;

			RemoveNext(instructions, idx);
		}
		else
		{
			i.opcode = SCOPE::kPush32;
		}
		break;

	case sizeof(Value64):
		if (next.opcode == SCOPE::kPush64)
		{
			i.opcode = SCOPE::kPush64Pair;

			i.param32 = Reinterpret<UInt32>(MakeTuple(Reinterpret<Int16>(i.param32), Reinterpret<Int16>(next.param32)));

			RemoveNext(instructions, idx);
		}
		else if (next.opcode == SCOPE::kPush32)
		{
			//this is useful for object[var32]

			i.opcode = SCOPE::kPush64and32;

			i.param32 = Reinterpret<UInt32>(MakeTuple(Reinterpret<Int16>(i.param32), Reinterpret<Int16>(next.param32)));

			RemoveNext(instructions, idx);
		}
		else if (next.opcode == OPCODE(optimisationPushConst32))
		{
			//this is useful for object#propertyid

			i.opcode = SCOPE::kPush64withConst32;

			i.param64 = next.param64;

			RemoveNext(instructions, idx);
		}
		else
		{
			i.opcode = SCOPE::kPush64;
		}
		break;
	}
}

template <class SCOPE> void Optimiser::ApplyRest_AssignValue(Instructions & instructions, UInt idx, Instruction & i, const Instruction & next)
{
	switch (GetVar(i).b)
	{
	case kSizeOf<Value8>:
		i.opcode = SCOPE::kAssignValue8;
		break;

	case kSizeOf<Value32>:
		i.opcode = SCOPE::kAssignValue32;
		break;

	case kSizeOf<Value64>:
		i.opcode = SCOPE::kAssignValue64;
		break;
	}
}

template <class SCOPE> void Optimiser::ApplyRest_AssignObject(Instructions & instructions, UInt idx, Instruction & i, const Instruction & next)
{
	bool st = !Reinterpret<TypeRef>(i.param64)->flags.Check(Type::kFlagThreadsafe);

	if constexpr (IsType<SCOPE,Member>::value)
	{
		if (st) i.opcode = SCOPE::kAssignObjectST;
	}
	else if (next.opcode == SCOPE::kPushObjectOpcode && i.param32 == next.param32)
	{
		i.opcode = st ? SCOPE::kAssignPushObjectST : SCOPE::kAssignPushObject;

		Optimiser::RemoveNext(instructions, idx);
	}
	else if (st)
	{
		i.opcode = SCOPE::kAssignObjectST;
	}
}

void Optimiser::ApplyRest(const Bindings & bindings, Array <Instruction> &instructions)
{
	LOCAL(void,ApplyReturnEllision)(bool object, Instructions & instructions, UInt & idx)
	{
		if (idx > 1)
		{
			auto i = instructions.GetData() + idx;

			auto & assign = (i)[-2];

			if (assign.opcode == (OPCODE(AssignLocalValue) + object))
			{
				auto & push = (i)[-1];

				if (push.opcode == OPCODE(PushLocal))
				{
					if (assign.param32 == push.param32)
					{
						instructions.Remove(idx - 2, 2);

						idx -= 2;
					}
				}
			}
		}
	}
	END

	INLINE(void, OptimiseSwitch8)(Instruction & instruction)
	{
		auto & table = *ToPointer<SwitchTable>(Reinterpret<UIntNative>(instruction.param64));

		auto & cases = table.cases;

		NonRefT <decltype(cases)> jumptable;

		jumptable.SetSize(17);

		jumptable.Fill(cases.GetLast());

		for (auto & i : ReverseSplice(cases, 1).a)
		{
			if (i.a > 15) return;

			jumptable[i.a] = i;
		}

		cases.Swap(jumptable);

		instruction.opcode = OPCODE(optimisationSwitch8JumpTable);
	}
	END

	INLINE(void, OptimiseIf)(bool test, Instruction & instruction, Opcode opcode)
	{
		if (test) instruction.opcode = opcode;
	}
	END

	const Tuple <UInt, Opcode> kReturnTypes[] =
	{
		{ 0, OPCODE(optimisationReturnVoid) },
		{ 1, OPCODE(optimisationReturn8) },
		{ 4, OPCODE(optimisationReturn32) },
		{ 8, OPCODE(optimisationReturn64) },
	};

	REFLEX_RLOOP(idx, instructions.GetSize())
	{
		auto & i = instructions[idx];

		auto & next = (&i)[1];

		switch (i.opcode)
		{
		case OPCODE(CallFn):
		{
			auto & fn = *ToPointer<ScriptFunction>(UIntNative(i.param64));

			i.opcode = (fn.flags2 & ScriptFunction::kFlagsNoObjects) ? OPCODE(optimisationCallFnOfValues) : OPCODE(CallFn);
		}
		break;

		case OPCODE(PushConst):
			switch (GetVar(i).b)
			{
			case 1:
				i.opcode = OPCODE(optimisationPushConst8);
				break;

			case 4:
				if (next.opcode == OPCODE(optimisationPushConst32))
				{
					i.opcode = OPCODE(optimisationPushConst64);

					i.param64 = (i.param64 & kMaxUInt32) | ((next.param64 & kMaxUInt32) << 32);

					RemoveNext(instructions, idx);
				}
				else
				{
					i.opcode = OPCODE(optimisationPushConst32);
				}
				break;

			case 8:
				if (next.opcode == OPCODE(intrinsicStringToKey32))
				{
					auto string = Cast<String>(ToPointer<Object>(UIntNative(i.param64)));

					i.opcode = OPCODE(optimisationPushConst32);
					i.param64 = string->hash.value;

					RemoveNext(instructions, idx);
				}
				else
				{
					i.opcode = OPCODE(optimisationPushConst64);
				}
				break;
			}
			break;

		case OPCODE(PushGlobal):
			ApplyRest_Push<Global>(instructions, idx, i, next);
			break;

		case OPCODE(PushLocal):
			ApplyRest_Push<Local>(instructions, idx, i, next);
			break;

		case OPCODE(PushMember):
			switch (GetVar(i).b)
			{
			case kSizeOf<Value8>:
				i.opcode = OPCODE(optimisationPushMember8);
				break;

			case kSizeOf<Value32>:
				i.opcode = OPCODE(optimisationPushMember32);
				break;

			case kSizeOf<Value64>:
				i.opcode = OPCODE(optimisationPushMember64);
				break;
			}
			break;

		case OPCODE(AssignGlobalValue):
			ApplyRest_AssignValue<Global>(instructions, idx, i, next);
			break;

		case OPCODE(AssignLocalValue):
			ApplyRest_AssignValue<Local>(instructions, idx, i, next);
			break;

		case OPCODE(AssignMemberValue):
			ApplyRest_AssignValue<Member>(instructions, idx, i, next);
			break;

		case OPCODE(AssignGlobalObject):
			ApplyRest_AssignObject<Global>(instructions, idx, i, next);
			break;

		case OPCODE(AssignLocalObject):
			ApplyRest_AssignObject<Local>(instructions, idx, i, next);
			break;

		case OPCODE(AssignMemberObject):
			ApplyRest_AssignObject<Member>(instructions, idx, i, next);
			break;

		case OPCODE(Switch8):
			OptimiseSwitch8::Call(i);
			break;

		case OPCODE(Return):
			if (auto t = SearchValue<KeyCompare>(ToView(kReturnTypes), i.param32))
			{
				i.opcode = t->b;
			}
			ApplyReturnEllision::Call(false, instructions, idx);
			break;

		case OPCODE(ReturnObject):
			if (!Reinterpret<TypeRef>(i.param64)->flags.Check(Type::kFlagThreadsafe))
			{
				i.opcode = OPCODE(optimisationReturnObjectST);
			}
			ApplyReturnEllision::Call(true, instructions, idx);
			break;

		case OPCODE(intrinsicValueEqual):
			switch (i.param64)
			{
			case kSizeOf<Value32>:
				i.opcode = OPCODE(optimisationValueEqual32);
				break;

			case kSizeOf<Value64>:
				i.opcode = OPCODE(optimisationValueEqual64);
				break;
			}
			break;

		case OPCODE(intrinsicValueInequal):
			switch (i.param64)
			{
			case kSizeOf<Value32>:
				i.opcode = OPCODE(optimisationValueInequal32);
				break;

			case kSizeOf<Value64>:
				i.opcode = OPCODE(optimisationValueInequal64);
				break;
			}
			break;

		case OPCODE(intrinsicIntegralArrayNext):
			switch (i.param64)
			{
			case kSizeOf<Value32>:
				i.opcode = OPCODE(optimisationValueArray32Next);
				break;

			case kSizeOf<Value64>:
				i.opcode = OPCODE(optimisationValueArray64Next);
				break;
			}
			break;

		case OPCODE(intrinsicObjectArrayNext):
			OptimiseIf::Call(i.param64, i, OPCODE(optimisationObjectArrayNextST));
			break;

		default:
			break;
		}
	}
}
	
void Optimiser::InlineFunctionCalls(bool local, LayoutTemplate & layout, Instructions & instructions)
{
	REFLEX_LOCAL(bool, CanInline)(ScriptFunction & fn)
	{
		for (auto & i : fn.instructions)
		{
			if (i.opcode == OPCODE(CallFn) && ToPointer<ScriptFunction>(UIntNative(i.param64)) == &fn) return false;
		}

		return true;
	}
	END

	REFLEX_LOCAL(void, ExpandCalls)(ScriptFunction & fn)
	{
		REFLEX_LOOP(a, fn.instructions.GetSize())
		{
			//auto instruction = fn.instructions[a];

			//auto & location = Reinterpret<Int>(instruction.param64);

//			if (location < 0 && Search(V(opcodes), instruction.opcode))
//			{
//				location += fn.arguments_size + 16;
//			}
//
//			instructions.Insert(idx, instruction);
		}
	}
	END


	//TODO
	//also need to add locals to parent scope (current did just arguments)

	Pair <Opcode> opcodes[] = 
	{
		{ OPCODE(PushLocal), OPCODE(PushLocal) },
		{ OPCODE(PushLocalByAdr), OPCODE(PushLocalByAdr) },
		{ OPCODE(AssignLocalValue), OPCODE(AssignLocalValue) },
		{ OPCODE(AssignLocalObject), OPCODE(AssignLocalObject) }
	};

	if (!local)
	{
		opcodes[0].b = OPCODE(PushGlobal);
		opcodes[1].b = OPCODE(PushGlobalByAdr);
		opcodes[2].b = OPCODE(AssignGlobalValue);
		opcodes[3].b = OPCODE(AssignGlobalObject);
	}

	UInt ti = 0;

	auto location = local ? Location::kLocal : Location::kGlobal;

	REFLEX_RLOOP(idx, instructions.GetSize())
	{
		auto & instruction = instructions[idx];

		if (instruction.opcode == OPCODE(CallFn))
		{
			auto & fn = *ToPointer<ScriptFunction>(UIntNative(instruction.param64));

			auto writepos = idx;

			if (CanInline::Call(fn))	//not inlined yet	//!BitCheck(instruction.param32, 31)
			{
				auto offset = GetArgumentsLocation(fn);

				Source src = { instruction.line, instruction.file };

				instructions.Remove(idx);

				Sequence <Int16,Int16> remap;

				for (auto & i : fn.args)
				{
					Int16 newlocation = 0;

					auto & type = *i.type;

					if (type.IsObject())
					{
						newlocation = layout.AddObject<true>(type, kArgNames[ti++ % 16], &REFLEX_NULL(Object));
					}
					else
					{
						newlocation = layout.AddValue(type, kArgNames[ti++ % 16], type.params);
					}

					remap.Insert(offset, newlocation);

					offset += i.type->size;

					Variable var = { &type, newlocation, location, false };

					instructions.Insert(idx, MakeAssignVariable(src, var));

					writepos++;
				}

				//TODO need to merge locals of called function to locals (or globals) of caller -> into layout template

				//for (auto & i : fn.layout->num_object)
				//{
				//	Int16 newlocation = 0;

				//	auto & type = *i.type;

				//	if (type.IsObject())
				//	{
				//		newlocation = layout.AddObject<true>(type, kArgNames[ti++ % 16], &REFLEX_NULL(Object));
				//	}
				//	else
				//	{
				//		newlocation = layout.AddValue(type, kArgNames[ti++ % 16], type.params);
				//	}

				//	remap.Insert(offset, newlocation);

				//	offset += i.type->size;

				//	Variable var = { &type, newlocation, location, false };

				//	instructions.Insert(idx, MakeAssignVariable(src, var));

				//	writepos++;
				//}

				REFLEX_RLOOP(a, fn.instructions.GetSize() - 1)
				{
					auto instruction = fn.instructions[a];

					if (auto newopcode = SearchValue<KeyCompare>(ToView(opcodes), instruction.opcode))
					{
						auto & location = Reinterpret<Int64>(instruction.param64);

						if (auto pnewlocation = remap.SearchValue(Int16(location)))
						{
							location = *pnewlocation;
						}

						instruction.opcode = newopcode->b;

						instructions.Insert(writepos, instruction);
					}
					else if (instruction.opcode == OPCODE(Return))
					{
					}
					else
					{
						instructions.Insert(writepos, instruction);
					}
				}
			}
		}
	}
}

void ProgramImpl::Optimise(const Compiler::State & state, LayoutTemplate & global, Instructions & global_code, Sequence <ScriptFunction*, LayoutTemplate> & functions)
{
	REFLEX_FOREACH(itr, functions)
	{
		auto & fn = *itr.key;

		auto & layout = itr.value;

		Optimiser::RemoveDiscards(fn.instructions);

		//Optimiser::InlineFunctionCalls(true, layout, fn.instructions);

		Optimiser::ApplyLocalCopyEllisions(state, layout, fn.instructions);
	}

	Optimiser::RemoveDiscards(global_instructions);

	//Optimiser::InlineFunctionCalls(false, global, global_instructions);

	Optimiser::ApplyRest(bindings, global_instructions);

	REFLEX_FOREACH(itr, functions)
	{
		auto & fn = *itr.key;

		//LinkExternalObjects::Call(bindings, fn.instructions);

		Optimiser::ApplyRest(bindings, /*itr.value,*/ fn.instructions);
	}

	//TODO literal comparisons on 0
	//optimisationPushConst32 + optimisationValueEqual32 => optimisationValueIsNull32
	//optimisationPushConst32 + optimisationValueInequal32 => optimisationValueIsNonNull32

	//TODO return const32
	//optimisationPushConst32 + optimisationReturn32 => optimisationReturnConst32


	//missing intrinsics
	//OP(intrinsicInt32LessThanOrEqual)		//TODO
	//OP(intrinsicInt32GreaterThan)
}

REFLEX_END_INTERNAL

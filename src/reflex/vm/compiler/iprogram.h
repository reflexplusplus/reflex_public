#pragma once

#include "error.h"




REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

REFLEX_INLINE void SetSource(Instruction & instruction, const Source & source)
{
	instruction.line = source.line;
	instruction.file = source.file;
}

REFLEX_INLINE Opcode GetVarOpcode(const Source & src, const Variable & var, const ArrayView <Opcode> & opcodes)
{
	Assume(UInt8(var.location) < opcodes.size, SyntaxError(src, kErrorInternalError));

	return opcodes[UInt8(var.location)];
}

template <class P1> REFLEX_INLINE UInt64 MakeParam64(P1 a)
{
	UInt64 t = 0;

	Reinterpret<P1>(t) = a;

	return t;
}

template <class P1, class P2> REFLEX_INLINE UInt32 MakeParam32(P1 a, P2 b)
{
	return Reinterpret<UInt32>(Pair<UInt16>({ UInt16(a), UInt16(b) }));
}

inline Instruction MakeAssignVariable(const Source & src, Variable var)	//load a local varible ontop of stack
{
	REFLEX_ASSERT(var);

	if (!And(!var.is_const, var.type->flags.Check(Type::kFlagAssignable))) FAIL(src, kErrorStageCompile, kErrorNonAssignableValue);

	auto & type = *var.type;

	Opcode opcode;
	
	if (type.IsObject())
	{
		opcode = GetVarOpcode(src, var, { OPCODE(AssignGlobalObject), OPCODE(AssignLocalObject), OPCODE(AssignMemberObject) });

		//instruction.flags = !type.flags.Check(Type::kFlagThreadsafe);
	}
	else
	{
		opcode = GetVarOpcode(src, var, { OPCODE(AssignGlobalValue), OPCODE(AssignLocalValue), OPCODE(AssignMemberValue) });

		//instruction.flags = type.size;
	}

	return { src.line, src.file, opcode, MakeParam32(var.address, type.size), MakeParam64(&type) };
}

inline void AddAssignVariable(Instructions & opcodes, const Source & src, Variable var)	//load a local varible ontop of stack
{
	opcodes.Push(MakeAssignVariable(src, var));
}

REFLEX_INLINE void AddCallExternal(Instructions & instructions, const Source & src, const ExternalFunction & fn, UInt narg)
{
	instructions.Push({ src.line, src.file, OPCODE(CallExternal), narg, ToUIntNative(fn.externalfnptr) });

	if (fn.clientdata)
	{
		instructions.Push({ src.line, src.file, OPCODE(Data), 0, ToUIntNative(&fn.clientdata) });
	}
}

REFLEX_INLINE TypeRef AddCall(Instructions & instructions, const Source & src, const Function & fn, UInt nargs)
{
	if (fn.type == Function::kTypeExternal)
	{
		AddCallExternal(instructions, src, Cast<ExternalFunction>(fn), nargs);
	}
	else
	{

		auto & instruction = instructions.Push({src.line, src.file, OPCODE(NumOpcode), 0, 0 });

		if (fn.type == Function::kTypeScript)
		{
			instruction.opcode = OPCODE(CallFn);

			instruction.param64 = ToUIntNative(&fn);
		}
		else if (fn.type == Function::kTypeIntrinsic)
		{
			auto & intrinsic = Cast<Intrinsic>(fn);

			instruction.opcode = intrinsic.opcode;

			instruction.param64 = intrinsic.param64;

			instruction.param32 = intrinsic.param32;
		}
	}

	return fn.rtn.type;
}

REFLEX_INLINE Key32 AddMarker(Instructions & instructions, const Source & src, Key32 markerid)
{
	instructions.Push({ src.line, src.file, OPCODE(Marker), 0, markerid.value });

	return markerid;
}

REFLEX_INLINE void AddJump(Instructions & opcodes, const Source & src, Key32 markerid)
{
	opcodes.Push({ src.line, src.file, OPCODE(Jump), 0, markerid.value });
}

REFLEX_INLINE void AddConditionalJump(Instructions & opcodes, const Source & src, Key32 markerid, UInt8 size, Opcode _8bit, Opcode _32bit)
{
	opcodes.Push({ src.line, src.file, size == 1 ? _8bit : _32bit, 0, markerid.value });
}

REFLEX_INLINE void AddSwitch(Instructions & opcodes, const Source & src, bool _32bit, SwitchTable & table)
{
	opcodes.Push({ src.line, src.file, _32bit ? OPCODE(Switch32) : OPCODE(Switch8), 0, ToUIntNative(&table) });
}

REFLEX_INLINE void AddReturn(Instructions & opcodes, const Source & src, const ScriptFunction & fn)
{
	auto offset = GetArgumentsLocation(fn);

	for (auto & i : fn.args)
	{
		offset += i.type->size;
	}

	auto return_t = fn.rtn.type;

	opcodes.Push({ src.line, src.file, return_t->IsObject() ? OPCODE(ReturnObject) : OPCODE(Return), return_t->size, ToUIntNative(&fn) });
}

REFLEX_INLINE void AddPushConst32(Instructions & opcodes, const Source & src, const UInt32 & value)
{
	opcodes.Push({ src.line, src.file, OPCODE(PushConst), MakeParam32(0, 4), value });
}

void AddPushConst(Instructions & opcodes, const Source & src, const Data::Archive::View & value)
{
	//REWORK_64
	//use data

	const UInt8 * ptr = value.data;

	auto nchunk = value.size / 8;

	REFLEX_LOOP(idx, nchunk)
	{
		auto & temp = *Reinterpret<UInt64>(ptr);

		opcodes.Push({ src.line, src.file, OPCODE(PushConst), MakeParam32(0, 8), temp });

		ptr += 8;
	}

	if (auto remainder = value.size % 8)
	{
		auto & instruction = opcodes.Push({ src.line, src.file, OPCODE(PushConst), MakeParam32(0, remainder), 0 });

		MemCopy(ptr, &instruction.param64, remainder);
	}
}

REFLEX_INLINE void AddPushNull(Instructions & opcodes, const Source & src, const Type * type)
{
	if (type->IsObject())
	{
		opcodes.Push({ src.line, src.file, OPCODE(intrinsicNullObject), 0, ToUIntNative(type) });
	}
	else
	{
		AddPushConst(opcodes, src, type->params);
	}
}

REFLEX_INLINE void AddPushNew(Instructions & opcodes, const Source & src, const Type * type, Opcode opcode = OPCODE(intrinsicNewObject))
{
	opcodes.Push({ src.line, src.file, opcode, 0, ToUIntNative(type) });
}

REFLEX_INLINE TypeRef AddPushVariable(Instructions & opcodes, const Source & src, Variable var, bool byref)
{
	Opcode opcode;

	if (byref)
	{
		opcode = GetVarOpcode(src, var, { OPCODE(PushGlobalByAdr), OPCODE(PushLocalByAdr), OPCODE(PushMemberByAdr) });
	}
	else
	{
		opcode = GetVarOpcode(src, var, { OPCODE(PushGlobal), OPCODE(PushLocal), OPCODE(PushMember), OPCODE(PushConst) });
	}

	opcodes.Push({ src.line, src.file, opcode, MakeParam32(var.address, var.type->size), MakeParam64(var.type)});

	return var.type;
}

REFLEX_INLINE void AddDiscardVariable(Instructions & opcodes, const Source & src, TypeRef type)
{
	if (auto size = type->size)
	{
		opcodes.Push({ src.line, src.file, OPCODE(Discard), size, 0 });
	}
}

REFLEX_INLINE void AddComparisonIntrinsic(Instructions & instructions, const Source & src, TypeRef type, Opcode opcode)
{
	instructions.Push({ src.line, src.file, opcode, 0, type->size });
}

REFLEX_INLINE void AddAbort(Instructions & instructions, const Source & src)
{
	instructions.Push({ src.line, src.file, OPCODE(Abort), 0, 0 });
}

REFLEX_END_INTERNAL

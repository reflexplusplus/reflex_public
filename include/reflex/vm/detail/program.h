#pragma once

#include "../error.h"
#include "stack.h"
#include "opcodes.h"





//
//all state needed for run time defined here

REFLEX_NS(Reflex::VM::Detail)

enum class Location : UInt8
{
	kGlobal,
	kLocal,
	kMember,
	kConst,
	kTemporary,	//means top of stack returned from function etc
};

struct Variable;

using NamedVariable = Pair <Key32,Variable>;

using Variables = Array <NamedVariable>;

REFLEX_END

REFLEX_NS(Reflex::VM)

using ExternalFunctionPtr = FunctionPointer <void(Context&)>;

REFLEX_END




//
//

struct Reflex::VM::Detail::Instruction
{
	UInt16 line;
	UInt8 file;
	UInt8 opcode;
	UInt32 param32;
	UInt64 param64;
};

struct Reflex::VM::Detail::Layout
{
	UInt16 size = 0;

	UInt16 num_object = 0;
};

struct Reflex::VM::Detail::Variable
{
	operator bool() const { return type; }


	TypeRef type = 0;

	Int16 address = 0;

	Location location = Location::kTemporary;

	bool is_const = false;
};

struct Reflex::VM::Argument
{
	enum Flags : UInt8
	{
		kFlagsReference = 1,
		kFlagsTemplate = 2,
		kFlagsOptional = 4,
	};

	Argument(TypeRef type = 0, bool byref = false, Key32 name = kNullKey, bool isconst = false)
		: type(type),
		byref(byref),
		isconst(isconst),
		name(name)
	{
	}

	bool operator==(const Argument & argument) const
	{
		REFLEX_ASSERT(type);	//enforce that lhs is always non template

		return And(Or(type == argument.type, !argument.type), byref == argument.byref);
	}

	bool operator!=(const Argument & argument) const { return Or(type != argument.type, byref != argument.byref/*, templateidx != argument.templateidx*/); }

	TypeRef type;
	bool byref;
	bool isconst;
	Key32 name;
};

struct Reflex::VM::Function
{
	enum Type : UInt8
	{
		kTypeIntrinsic,
		kTypeExternal,
		kTypeScript,
	};

	enum Flags : UInt8
	{
		kFlagsMt = 1,	//RESERVED for future, wont remove as use of flags delicate
		kFlagsMember = 2,
	};

	Type type;

	UInt8 flags2 = 0;

	Symbol symbol;

	StaticString name;

	Array <Argument> targs;			//for specificity/selection by template arg e.g. DoIt@Int32()

	Argument rtn;

	Array <Argument> args;		//TypeRef,byreference,name

	UInt64 args_hash;
};

struct Reflex::VM::Intrinsic : public Function
{
	UInt8 opcode;
	UInt32 param32;
	UInt64 param64;
};

struct Reflex::VM::ExternalFunction : public Function
{
	enum Flags : UInt8
	{
		kFlagsVaradic = 4,
		kFlagsAssociative = 8,	//only used at template stage
	};

	ExternalFunctionPtr externalfnptr;

	Data::Archive clientdata;
};

struct Reflex::VM::ScriptFunction : public Function
{
	enum Flags : UInt8
	{
		kFlagsNoObjects = 16,	//hint for compiler
	};

	Detail::Layout * layout;

	UInt16 arguments_size;

	Array <Detail::Instruction> instructions;
};

struct Reflex::VM::ExternalObject
{
	Symbol symbol;
	TypeRef type;
	Object * object;
	bool retain;
};

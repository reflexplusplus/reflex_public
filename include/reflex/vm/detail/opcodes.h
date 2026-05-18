#pragma once




//
//

#define OPCODE(CODE) REFLEX_CONCATENATE(k,CODE)

REFLEX_NS(Reflex::VM)

enum Opcode : UInt8
{
	#define OP(CODE) REFLEX_CONCATENATE(k,CODE),
	#include "[opcodes].h"
	#undef OP

	kAbort,	//last instruction, normal termination
	kEnd,	//last instruction, normal termination
	kNumOpcode
};

REFLEX_END

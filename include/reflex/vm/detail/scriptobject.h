#pragma once

#include "stack.h"
#include "../compiler.h"




//
//

REFLEX_NS(Reflex::VM::Detail)

struct ScriptObject : public Reflex::Object
{
	static UInt GetDataOffset(ContainerType type);

	static TypeRef RegisterType(Compiler::State & bindings, const WString::View & path, Key32 ns, StaticString name, Variables && members, const void * layout_template, bool mt = true);
};

REFLEX_END

#pragma once

#include "compiler.h"




//
//

REFLEX_NS(Reflex::VM)

class Module;

REFLEX_END




//
//

class Reflex::VM::Module : public Reflex::Detail::StaticItem <Module>
{
public:

	using Instantiator = FunctionPointer <void(Compiler::State&,UInt8,Object&)>;


	Module(StaticString id, const ArrayView <ConstTRef<Module>> & dependencies, UInt8 context_flags, Instantiator instantiator) : name(id), id(id), dependencies(reserved, dependencies.size), context_flags(context_flags), instantiator(instantiator)
	{
		REFLEX_ASSERT(dependencies.size <= GetArraySize(reserved));

		auto preserved = reserved;

		for (auto & i : dependencies) *preserved++ = i;
	}


	const StaticString name;
	
	const Key32 id;

	const ArrayView < ConstTRef <Module> > dependencies;

	const UInt8 context_flags;

	const Instantiator instantiator;

	ConstTRef <Module> reserved[4] = { kNoValue, kNoValue, kNoValue, kNoValue };
};

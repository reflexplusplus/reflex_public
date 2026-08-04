#pragma once

#include "forward.h"




//
//Module

#if DOCGEN

class Docgen::Module : public Reflex::Detail::StaticItem <Module>
{
public:

	Module(Key32 language, Key32 codebase, const ArrayView < ConstTRef <Module> > dependencies, const FunctionPointer <void(Writer & writer)> instantiate);


	const Key32 language, codebase;

	const ArrayView < ConstTRef <Module> > dependencies;

	const FunctionPointer <void(Writer & writer)> instantiate;

	ConstTRef <Module> reserved[4] = { kNoValue, kNoValue, kNoValue, kNoValue };
};

#else

class Docgen::Module
{
	Module(Key32 language, Key32 codebase, const ArrayView < ConstTRef <Module> > dependencies, const FunctionPointer <void(Writer & writer)> instantiate) {}
};

#endif




//
//impl

#if DOCGEN

inline Docgen::Module::Module(Key32 language, Key32 codebase, const ArrayView < ConstTRef <Module> > dependencies, const FunctionPointer <void(Writer & writer)> instantiate)
	: language(language)
	, codebase(codebase)
	, dependencies({ reserved, dependencies.size })
	, instantiate(instantiate)
{
	REFLEX_ASSERT(dependencies.size <= GetArraySize(reserved));

	auto preserved = reserved;

	for (auto & i : dependencies) *preserved++ = i;
}

#endif

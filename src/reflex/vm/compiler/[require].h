#pragma once

#include "../[require].h"

REFLEX_NS(Reflex::VM::Detail)

REFLEX_INLINE UInt64 HashArgs(const ArrayView <Argument> & targs, const ArrayView <Argument> & args)
{
	Pair <UInt32> hash = { kHashSeed, kHashSeed };

	for (auto & i : targs) Reflex::Detail::IncrementHash(hash.a, i.type->type_id);

	for (auto & i : args) Reflex::Detail::IncrementHash(hash.b, i.type->type_id);

	return Reinterpret<UInt64>(hash);
}

REFLEX_END
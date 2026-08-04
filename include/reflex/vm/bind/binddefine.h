#pragma once

#include "bind.h"




//
//

#define VM_BIND_CONST(SYMBOL, state, type, ns, name) state.RegisterConstant(type, ns, name, Reflex::Data::Pack(SYMBOL));

#define VM_BIND_ENUM(cstate, NS, NAME) Reflex::VM::BindEnumMember(cstate, K32(REFLEX_STRINGIFY(NS)), REFLEX_STRINGIFY(NAME), Reflex::UInt8(NS::NAME))

#define VM_BEGIN_ENUM(TBINDINGS, NS, name) { auto & _tbindings = TBINDINGS; [[maybe_unused]] Reflex::VM::Symbol values[] = {
#define VM_QBIND_ENUM(NS, NAME) VM_BIND_ENUM(_tbindings, NS, NAME)
#define VM_END_ENUM };}

REFLEX_NS(Reflex::VM)

template <class TYPE> inline Symbol BindConstant(Compiler::State & state, Key32 ns, StaticString name, TYPE value)
{
	if constexpr (sizeof(TYPE) == 1)
	{
		return state.RegisterConstant(state.bindings->uint8_t, ns, name, Data::Pack(UInt8(value)));
	}
	else if constexpr (IsType<TYPE,UInt32,Int32>::value)
	{
		return state.RegisterConstant(state.bindings->int32_t, ns, name, Data::Pack(value));
	}
	else if constexpr (IsType<TYPE,Float32>::value)
	{
		return state.RegisterConstant(state.bindings->float32_t, ns, name, Data::Pack(value));
	}
	else if constexpr (IsType<TYPE,Key32>::value)
	{
		return state.RegisterConstant(state.bindings->key32_t, ns, name, Data::Pack(value));
	}
}

inline Symbol BindEnumMember(Compiler::State & state, Key32 ns, StaticString name, UInt8 value)
{
	return state.RegisterConstant(state.bindings->uint8_t, ns, name, Data::Pack(value));
}

REFLEX_END

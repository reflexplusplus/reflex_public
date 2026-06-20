#pragma once

#include "auxtypes.h"




//
//Secondary API

namespace Reflex
{

	template <class TYPE, UInt SIZE> inline constexpr UInt GetArraySize(const TYPE(&data)[SIZE]) noexcept { return SIZE; }

	template <class TYPE> inline TYPE Copy(const TYPE & value) { return value; }

	template <class TYPE> constexpr TYPE & Deref(TYPE & type) { return type; }

	template <class TYPE> constexpr TYPE & Deref(TYPE * type) { return *type; }

	template <class TYPE> constexpr TYPE * GetAdr(TYPE & type) { return &type; }

}




//
//legacy

REFLEX_NS(Reflex)

template <class TYPE> [[deprecated]] inline NonRefT <TYPE> && Move(TYPE && value) noexcept { return static_cast<NonRefT<TYPE>&&>(value); }

REFLEX_END

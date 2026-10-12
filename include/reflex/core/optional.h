#pragma once

#include "meta/traits.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE> struct Optional;

}




//
//Optional

template <class TYPE>
struct Reflex::Optional
{
	Optional() = default;

	Optional(NoValue) : set(false) {}

	Optional(const TYPE & value) : value(value), set(true) {}

	Optional(const TYPE & value, bool set) : value(value), set(set) {}

	void Set(const TYPE & value) { Optional::value = value; set = true; }

	void Unset() { Optional::value = {}; set = false; }

	explicit operator bool() const { return set; }

	bool operator==(const Optional & value) const = default;


	TYPE value = {};

	bool set = false;
};

REFLEX_SET_TRAIT_TEMPLATED(Optional, IsBoolCastable)
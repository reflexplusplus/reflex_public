#pragma once

#include "defines.h"
#include "meta/traits.h"




//
//Primary API

namespace Reflex
{

	class Idx;

}




//
//Idx

class Reflex::Idx
{
public:

	//lifetime

	constexpr Idx();

	constexpr Idx(const Idx & idx) = default;

	constexpr Idx(UInt value);



	//assign

	Idx & operator=(const Idx & idx) = default;



	//logic

	explicit operator bool() const;

	bool operator==(const Idx & idx) const = default;

	bool operator!=(const Idx & idx) const = default;



	UInt value;

};

REFLEX_SET_TRAIT_EX(Idx, IsClass, false);

REFLEX_SET_TRAIT(Idx, IsBoolCastable);




//
//impl

REFLEX_ASSERT_RAW(Reflex::Idx);

REFLEX_INLINE constexpr Reflex::Idx::Idx()
	: value(kMaxUInt32)
{
}

REFLEX_INLINE constexpr Reflex::Idx::Idx(UInt value)
	: value(value)
{
}

REFLEX_INLINE Reflex::Idx::operator bool() const
{
	return value != kMaxUInt32;
}

REFLEX_NS(Reflex)

constexpr Idx kInvalidIdx(kMaxUInt32);

template <class TO> void Reinterpret(Idx idx);	//intentionally prevent

REFLEX_END

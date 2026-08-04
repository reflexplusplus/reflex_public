#pragma once

#include "array/defines.h"




//
//Primary API

namespace Reflex
{

	template <class UINT> class Key;


	using Key64 = Key <UInt64>;

	using Key32 = Key <UInt32>;


	template <class UINT> bool True(Key <UINT> value);

}

#define REFLEX_DECLARE_KEY32(ID) static constexpr auto k##ID = K32(REFLEX_STRINGIFY(ID))




//
//Key

template <class UINT>
class Reflex::Key
{
public:

	//types

	using Type = UINT;



	//lifetime

	constexpr Key();

	constexpr Key(const Key & value) = default;

	constexpr Key(UINT value);


	consteval Key(const char * s);

	template <UInt N> consteval Key(const char(&s)[N]);


	Key(const Array <char> & string);

	Key(const ArrayView <char> & string);


	Key(const Array <WChar> & string);

	Key(const ArrayView <WChar> & string);



	//access

	constexpr Key & operator=(const Key & value) = default;


	bool operator==(const Key & key) const;

	bool operator!=(const Key & key) const;

	bool operator<(const Key & key) const;


	constexpr bool operator==(UINT value) const;

	constexpr bool operator!=(UINT value) const;



	UINT value;

};

REFLEX_SET_TRAIT_TEMPLATED(Key, IsBoolCastable);
REFLEX_SET_TRAIT_TEMPLATED(Key, IsTrivial);

//namespace Reflex { template <class UINT> struct IsBoolCastable < Key <UINT> > { static constexpr bool value = true; }; }




//
//impl

REFLEX_ASSERT_RAW(Reflex::Key32);
REFLEX_ASSERT_RAW(Reflex::Key64);

REFLEX_NS(Reflex)

constexpr UInt32 kHashSeed = 5381;

REFLEX_END

template <class UINT> inline constexpr Reflex::Key<UINT>::Key()
	: value(kHashSeed)
{
}

template <class UINT> inline constexpr Reflex::Key<UINT>::Key(UINT value)
	: value(value) 
{
}

template <class UINT> REFLEX_INLINE bool Reflex::Key<UINT>::operator==(const Key & key) const
{
	return Key::value == key.value;
}

template <class UINT> REFLEX_INLINE bool Reflex::Key<UINT>::operator!=(const Key & key) const
{
	return Key::value != key.value;
}

template <class UINT> REFLEX_INLINE bool Reflex::Key<UINT>::operator<(const Key & key) const
{
	return Key::value < key.value;
}

template <class UINT> inline constexpr bool Reflex::Key<UINT>::operator==(UINT value) const
{
	return Key::value == value;
}

template <class UINT> inline constexpr bool Reflex::Key<UINT>::operator!=(UINT value) const
{
	return Key::value != value;
}

template <class UINT> REFLEX_INLINE bool Reflex::True(Key <UINT> key)
{
	return bool(key);
}

namespace Reflex { template <class UINT> struct IsClass < Key <UINT> > { static constexpr bool value = false; }; }

REFLEX_NS(Reflex)	//this is here due to VC ordering BUG

constexpr Key32 kNullKey = kHashSeed;

constexpr Key32 kZeroKey = UInt32(0ul);

inline constexpr bool IsValidKey(Key32 key) { return key.value != kHashSeed; }

inline constexpr bool IsNullKey(Key32 key) { return key.value == kHashSeed; }

REFLEX_END
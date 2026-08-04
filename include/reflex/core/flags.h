#pragma once

#include "assert.h"
#include "functions/bit.h"
#include "functions/logic.h"




//
//Deprecated API

namespace Reflex
{

	template <class UINT> class Flags;


	using Flags8 = Flags <UInt8>;

	using Flags16 = Flags <UInt16>;

	using Flags32 = Flags <UInt>;

	using Flags64 = Flags <UInt64>;


	template <class UINT> Flags <UINT> operator|(const Flags <UINT> & a, const Flags <UINT> & b);

	template <class UINT> Flags <UINT> operator&(const Flags <UINT> & a, const Flags <UINT> & b);

}




//
//Flags

template <class UINT>
class Reflex::Flags
{
public:

	//lifetime

	Flags();

	Flags(const Flags & flags) = default;

	Flags(UINT word);



	//set all

	void Clear();

	void Fill();

	void Flip();



	//check all

	bool Empty() const;

	bool Full() const;


	UInt8 Count() const;


	Idx GetFirst() const;

	Idx GetLast() const;



	//set bit

	void Clear(UInt bit);

	void Set(UInt bit);

	void Set(UInt bit, bool value);

	void Flip(UInt bit);

	void Or(UInt bit, bool value);



	//check bit

	bool Check(UInt bit) const;

	bool operator[](UInt bit) const;



	//combine

	Flags & operator|=(const Flags & flags);

	Flags & operator|=(UINT word);


	Flags & operator&=(const Flags & flags);

	Flags & operator&=(UINT word);



	//compare

	explicit operator bool() const;

	bool operator==(const Flags & flags) const;

	bool operator==(UINT word) const;

	bool operator!=(const Flags & flags) const;

	bool operator!=(UINT word) const;

	template <class TYPE> bool operator==(const TYPE & value) const;	//dont compile

	template <class TYPE> bool operator!=(const TYPE & value) const;//dont compile



	//assign

	Flags & operator=(const Flags & flags) = default;

	Flags & operator=(UINT word);



	//export

	void SetWord(UINT word);

	const UINT & GetWord() const;

	UINT PollWord() volatile;


	operator UINT&();

	operator const UINT&() const;



private:

	UINT m_bits;

};

REFLEX_SET_TRAIT_TEMPLATED(Flags, IsBoolCastable);




//
//impl

REFLEX_ASSERT_RAW(Reflex::Flags8);

template <class UINT> REFLEX_INLINE Reflex::Flags<UINT>::Flags()
	: m_bits(0)
{
}

template <class UINT> REFLEX_INLINE Reflex::Flags<UINT>::Flags(UINT word)
	: m_bits(word)
{
}

template <class UINT> REFLEX_INLINE void Reflex::Flags<UINT>::Clear()
{
	m_bits = 0;
}

template <class UINT> REFLEX_INLINE void Reflex::Flags<UINT>::Fill()
{
	m_bits = UINT(-1);
}

template <class UINT> REFLEX_INLINE void Reflex::Flags<UINT>::Flip()
{
	m_bits = m_bits ^ UINT(-1);
}

template <class UINT> REFLEX_INLINE bool Reflex::Flags<UINT>::Empty() const
{
	return m_bits == 0;
}

template <class UINT> REFLEX_INLINE bool Reflex::Flags<UINT>::Full() const
{
	return m_bits == UINT(-1);
}

template <class UINT> REFLEX_INLINE Reflex::UInt8 Reflex::Flags<UINT>::Count() const
{
	return BitCount(m_bits);
}

template <class UINT> REFLEX_INLINE Reflex::Idx Reflex::Flags<UINT>::GetFirst() const
{
	return GetFirstBit(m_bits);
}

template <class UINT> REFLEX_INLINE Reflex::Idx Reflex::Flags<UINT>::GetLast() const
{
	return GetLastBit(m_bits);
}

template <class UINT> REFLEX_INLINE void Reflex::Flags<UINT>::Clear(UInt bit)
{
	m_bits = BitClear(m_bits, bit);
}

template <class UINT> REFLEX_INLINE void Reflex::Flags<UINT>::Set(UInt bit)
{
	m_bits = BitSet(m_bits, bit);
}

template <class UINT> REFLEX_INLINE void Reflex::Flags<UINT>::Set(UInt bit, bool value)
{
	m_bits = BitSet(m_bits, bit, value);
}

template <class UINT> REFLEX_INLINE void Reflex::Flags<UINT>::Flip(UInt bit)
{
	m_bits = BitFlip(m_bits, bit);
}

template <class UINT> REFLEX_INLINE void Reflex::Flags<UINT>::Or(UInt bit, bool value)
{
	m_bits |= (UINT(value) << bit);
}

template <class UINT> REFLEX_INLINE bool Reflex::Flags<UINT>::Check(UInt bit) const
{
	return BitCheck(m_bits, bit);
}

template <class UINT> REFLEX_INLINE bool Reflex::Flags<UINT>::operator[](UInt bit) const
{
	return BitCheck(m_bits, bit);
}

template <class UINT> REFLEX_INLINE Reflex::Flags <UINT> & Reflex::Flags<UINT>::operator|=(const Flags & flags)
{
	m_bits |= flags.m_bits;

	return *this;
}

template <class UINT> REFLEX_INLINE Reflex::Flags <UINT> & Reflex::Flags<UINT>::operator|=(UINT word)
{
	m_bits |= word;

	return *this;
}

template <class UINT> REFLEX_INLINE Reflex::Flags <UINT> & Reflex::Flags<UINT>::operator&=(const Flags & flags)
{
	m_bits &= flags.m_bits;

	return *this;
}

template <class UINT> REFLEX_INLINE Reflex::Flags <UINT> & Reflex::Flags<UINT>::operator&=(UINT word)
{
	m_bits &= word;

	return *this;
}

//template <class UINT> REFLEX_INLINE Flags <UINT> & Flags<UINT>::operator=(const Flags & flags)
//{
//	m_bits = flags.m_bits;
//
//	return *this;
//}

template <class UINT> REFLEX_INLINE Reflex::Flags <UINT> & Reflex::Flags<UINT>::operator=(UINT word)
{
	m_bits = word;

	return *this;
}

template <class UINT> REFLEX_INLINE Reflex::Flags<UINT>::operator bool() const
{
	return True(m_bits);
}

template <class UINT> REFLEX_INLINE bool Reflex::Flags<UINT>::operator==(const Flags & flags) const
{
	return m_bits == flags.m_bits;
}

template <class UINT> REFLEX_INLINE bool Reflex::Flags<UINT>::operator==(UINT word) const
{
	return m_bits == word;
}

template <class UINT> REFLEX_INLINE bool Reflex::Flags<UINT>::operator!=(const Flags & flags) const
{
	return m_bits != flags.m_bits;
}

template <class UINT> REFLEX_INLINE bool Reflex::Flags<UINT>::operator!=(UINT word) const
{
	return m_bits != word;
}

template <class UINT> REFLEX_INLINE void Reflex::Flags<UINT>::SetWord(UINT word)
{
	m_bits = word;
}

template <class UINT> REFLEX_INLINE const UINT & Reflex::Flags<UINT>::GetWord() const
{
	return m_bits;
}

template <class UINT> REFLEX_INLINE UINT Reflex::Flags<UINT>::PollWord() volatile
{
	UINT bits = m_bits;

	m_bits = 0;

	return bits;
}

template <class UINT> REFLEX_INLINE Reflex::Flags<UINT>::operator UINT&()
{
	return m_bits;
}

template <class UINT> REFLEX_INLINE Reflex::Flags<UINT>::operator const UINT&() const
{
	return m_bits;
}

template <class UINT> REFLEX_INLINE Reflex::Flags <UINT> Reflex::operator|(const Flags <UINT> & a, const Flags <UINT> & b)
{
	return a.GetWord() | b.GetWord();
}

template <class UINT> REFLEX_INLINE Reflex::Flags <UINT> Reflex::operator&(const Flags <UINT> & a, const Flags <UINT> & b)
{
	return a.GetWord() & b.GetWord();
}

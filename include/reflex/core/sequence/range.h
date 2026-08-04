#pragma once

#include "sequence.h"




//
//impl

REFLEX_NS(Reflex)

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS>
template <bool CONST>
struct Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::RangeImpl
{
	using Itr = typename Sequence::template ItrImpl <CONST>;

	using ReverseItr = Detail::ReverseItr <Itr>;

	using SequenceType = ConditionalType <CONST,const Sequence,Sequence>;

	RangeImpl(SequenceType & sequence)
		: m_sequence(&sequence)
		, m_begin(0)
		, m_end(sequence.GetSize())
	{
	}

	RangeImpl(SequenceType & sequence, const KEY & from_inclusive, const KEY & to)
		: m_sequence(&sequence)
		, m_begin(sequence.SearchGTE(from_inclusive).value)
		, m_end(Max<UInt32>(m_begin, sequence.SearchLT(to).value + 1))
	{
		REFLEX_ASSERT(!COMPARE::lt(to, from_inclusive));
	}

	operator bool() const {return m_end != m_begin; }

	UInt GetSize() const { return m_end - m_begin; }


	Itr begin() const { return Itr(*m_sequence, m_begin); }

	Itr end() const { return Itr(*m_sequence, m_end); }

	Detail::ReverseItr <Itr> rbegin() const { return Itr(*m_sequence, m_end); }

	Detail::ReverseItr <Itr> rend() const { return Itr(*m_sequence, m_begin); }



private:

	SequenceType * m_sequence;

	UInt m_begin;

	UInt m_end;
};


template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> auto MakeRange(Sequence <KEY,VALUE,COMPARE,CONTIGUOUS> & sequence, const KEY & lowest, const KEY & highest) { return typename Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Range(sequence, lowest, highest); }

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> auto MakeRange(const Sequence <KEY,VALUE,COMPARE,CONTIGUOUS> & sequence, const KEY & lowest, const KEY & highest) { return typename Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::ConstRange(sequence, lowest, highest); }



template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS, bool CONST> auto Reverse(typename Sequence <KEY,VALUE,COMPARE,CONTIGUOUS>::template RangeImpl <CONST> && iterable)
{
	return RangeHolder(iterable.begin(), iterable.end());
}

REFLEX_END

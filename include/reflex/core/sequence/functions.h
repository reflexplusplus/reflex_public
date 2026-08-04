#pragma once

#include "sequence.h"




//
//Secondary API

namespace Reflex
{

	template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> void RemoveKey(Sequence <KEY,VALUE,COMPARE,CONTIGUOUS> & sequence, const KEY & key);

	template <class POLICY = StandardCompare, class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> void RemoveValue(Sequence <KEY,VALUE,COMPARE,CONTIGUOUS> & sequence, const VALUE & value);


	template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> void Swap(Sequence <KEY,VALUE,COMPARE,CONTIGUOUS> & a, Sequence <KEY,VALUE,COMPARE,CONTIGUOUS> & b);

}




//
//impl

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline void Reflex::RemoveKey(Sequence <KEY,VALUE,COMPARE,CONTIGUOUS> & sequence, const KEY & key)
{
	while (auto idx = sequence.Search(key)) sequence.Remove(idx.value);
}

template <class POLICY, class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline void Reflex::RemoveValue(Sequence <KEY,VALUE,COMPARE,CONTIGUOUS> & sequence, const VALUE & value)
{
	REFLEX_RLOOP(idx, sequence.GetSize())
	{
		if (POLICY::eq(sequence[idx].value, value)) sequence.Remove(idx);
	}
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE void Reflex::Swap(Sequence <KEY,VALUE,COMPARE,CONTIGUOUS> & a, Sequence <KEY,VALUE,COMPARE,CONTIGUOUS> & b)
{
	a.Swap(b);
}

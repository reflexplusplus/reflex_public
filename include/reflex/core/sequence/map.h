#pragma once

#include "sequence.h"




//
//Primary API

namespace Reflex
{

	template <class KEY, class VALUE = NullType, class COMPARE = StandardCompare, bool CONTIGUOUS = false> class Map;

}




//
//Map

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS>
class Reflex::Map : protected Sequence <KEY,VALUE,COMPARE,CONTIGUOUS>
{
public:

	typedef Sequence <KEY,VALUE,COMPARE,CONTIGUOUS> Sequence;

	typedef typename Sequence::Item Item;



	//lifetime

	using Sequence::Sequence;



	//content

	using Sequence::Allocate;


	using Sequence::Clear;


	using Sequence::Set;

	using Sequence::Acquire;

	bool Unset(const KEY & key);


	using Sequence::GetFirst;

	using Sequence::GetLast;

	VALUE & operator[](const KEY & key) { return Sequence::Acquire(key); }


	VALUE * Search(const KEY & key, VALUE * fallback = nullptr) { return Sequence::SearchValue(key, fallback); }

	const VALUE * Search(const KEY & key, const VALUE * fallback = nullptr) const { return Sequence::SearchValue(key, fallback); }



	//advanced

	using Sequence::SearchGTE;

	using Sequence::SearchLT;

	using Sequence::GetData;

	using Sequence::GetSize;



	//assignment

	void Swap(Map & map) { Sequence::Swap(map); }



	//info
	
	using Sequence::operator bool;

	using Sequence::Empty;


	bool operator==(const Map & value) const { return Sequence::operator==(value); }

	bool operator!=(const Map & value) const { return Sequence::operator!=(value); }



	//iterate

	using Sequence::begin;

	using Sequence::end;

	using Sequence::rbegin;

	using Sequence::rend;

};

namespace Reflex { template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> struct IsBoolCastable < Map <KEY, VALUE, COMPARE, CONTIGUOUS> > { static constexpr bool value = true; }; }




//
//impl

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline bool Reflex::Map<KEY,VALUE,COMPARE,CONTIGUOUS>::Unset(const KEY & key)
{
	if (auto idx = Sequence::Search(key))
	{
		Sequence::Remove(idx.value);

		return true;
	}
	else
	{
		return false;
	}
}

#pragma once

#include "../types.h"




//
//Detail

namespace Reflex::Detail
{

	template <class TYPE>
	struct ReverseItr
	{
		ReverseItr(TYPE itr) : itr(itr) { --this->itr; }

		void operator++() { --itr; }

		auto & operator*() const { return *itr; }

		bool operator!=(const ReverseItr & b) const { return itr != b.itr; }

		TYPE itr;
	};

	template <typename TYPE>
	struct RangeHolder
	{
		RangeHolder(TYPE && begin = {}, TYPE && end = {}) : i(begin), e(end) {}

		auto begin() { return i; }

		auto end() { return e; }

		TYPE i, e;
	};

}

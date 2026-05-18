#pragma once

#include "../array.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE> inline ArrayRegion <TYPE> ToRegion(const ArrayRegion <TYPE> & value) { return value; }

	template <class TYPE> inline ArrayRegion <TYPE> ToRegion(Array <TYPE> & value) { return ArrayRegion<TYPE>(value.GetData(), value.GetSize()); }

	template <class TYPE> inline ArrayRegion <TYPE> ToRegion(ObjectOf < Array <TYPE> > & object) { return ToRegion(object.value); }

	template <class TYPE, UInt SIZE> inline constexpr ArrayRegion <TYPE> ToRegion(TYPE(&data)[SIZE]) { return { data, SIZE - IsNullTerminated<TYPE>::value }; }


	template <class TYPE> inline const ArrayView <TYPE> & ToView(const ArrayView <TYPE> & value) { return value; }

	template <class TYPE> inline ArrayView <TYPE> ToView(const ArrayRegion <TYPE> & value) { return { value.data, value.size }; }

	template <class TYPE> inline ArrayView <TYPE> ToView(const Array <TYPE> & value) { return ArrayView<TYPE>(value.GetData(), value.GetSize()); }

	template <class TYPE> inline ArrayView <TYPE> ToView(const TYPE & value) { return { &value, 1 }; }

	template <class TYPE> inline ArrayView <TYPE> ToView(const std::initializer_list <TYPE> & list) { return { list.begin(), UInt(list.size()) }; }

	template <class TYPE, UInt SIZE> inline constexpr ArrayView <NonConstT<TYPE>> ToView(TYPE(&data)[SIZE])
	{
		static_assert(!(!IsConst<TYPE>::value && IsNullTerminated<NonConstT<TYPE>>::value), "ToView with non-const char ptr is not-permitted. If its null-terminated, use ToView((const char*)ptr)");

		return { data, UInt(SIZE - IsNullTerminated<NonConstT<TYPE>>::value) };
	}


	inline ArrayView <char> ToView(const char * value) { return value; };

	inline ArrayView <WChar> ToView(const WChar * value) { return value; };


	template <class TYPE> inline ArrayView <TYPE> ToView(const ObjectOf < Array <TYPE> > & object) { return ToView(object.value); }

	template <class TYPE> inline auto ToView(TRef <TYPE> tref) { return ToView(*tref); }

	template <class TYPE> inline auto ToView(const Reference <TYPE> & reference) { return ToView(*reference); }


	template <class TYPE, class auto_1> Array <TYPE> ToArray(auto_1 && iterable);

}




//
//impl

template <class TYPE> inline Reflex::Array<TYPE>::Array(const TYPE * nullterminated) : Array(View(nullterminated), g_default_allocator) { REFLEX_STATIC_ASSERT(kIsNullTerminated); }

template <class TYPE, class auto_1> inline Reflex::Array <TYPE> Reflex::ToArray(auto_1 && iterable)
{
	Array <TYPE> rtn;

	auto itr = iterable.begin();

	auto end = iterable.end();

	rtn.Allocate(UInt(end - itr));

	while (itr != end)
	{
		rtn.template Push<kAllocateNone>(*itr++);
	}

	return rtn;
}

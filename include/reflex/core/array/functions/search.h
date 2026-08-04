#pragma once

#include "../../compare_policy.h"
#include "convert.h"
#include "../../idx.h"




//
//Primary API

namespace Reflex
{

	template <class POLICY = StandardCompare, class ARRAY, class VALUE> Idx Search(const ARRAY & array, VALUE && element_or_array);

	template <class POLICY = StandardCompare, class ARRAY, class VALUE> Idx ReverseSearch(const ARRAY & array, VALUE && element_or_array);


	template <class POLICY = StandardCompare, class VALUE, class ARRAY> Detail::ArrayItemType <ARRAY> * SearchValue(ARRAY & iterable, VALUE && element, Detail::ArrayItemType <ARRAY> * null = nullptr);

	template <class POLICY = StandardCompare, class VALUE, class ARRAY> Detail::ArrayItemType <const ARRAY> * SearchValue(const ARRAY & iterable, VALUE && element, Detail::ArrayItemType <const ARRAY> * null = nullptr);


	template <class POLICY = StandardCompare, class ARRAY, class FROM_VALUE, class TO_VALUE> auto Replace(const ARRAY & array, const FROM_VALUE & from_element_or_array, const TO_VALUE & to_element_or_array);


	template <class POLICY = StandardCompare, class TYPE, class VALUE> [[deprecated("use Remove")]] void RemoveValue(Array <TYPE> &array, VALUE && value);

	template <class POLICY = StandardCompare, class TYPE, class VALUE> void Remove(Array <TYPE> & array, VALUE && element_or_array);

	template <class POLICY = StandardCompare, class ARRAY, class VALUE> auto Filter(const ARRAY & array, VALUE && element_or_array);

}




//
//impl

REFLEX_NS(Reflex::Detail)

template <class POLICY, class TYPE, class VALUE> REFLEX_INLINE Idx SearchElement(const ArrayView <TYPE> & view, VALUE && value)
{
	for (auto & i : view)
	{
		if (POLICY::eq(i, std::forward<VALUE>(value))) return UInt(&i - view.data);
	}

	return {};
}

template <class POLICY, class TYPE, class VALUE> REFLEX_INLINE Idx ReverseSearchElement(const ArrayView <TYPE> & view, VALUE && value)
{
	REFLEX_RFOREACH(itr, view)
	{
		if (POLICY::eq(itr, std::forward<VALUE>(value))) return UInt(&itr - view.data);
	}

	return {};
}

template <class POLICY, class TYPE, class VALUE> inline TYPE * SearchValue(const ArrayRegion <TYPE> & haystack, VALUE && value, TYPE * null)
{
	auto itr = haystack.begin();

	auto end = haystack.end();

	while (itr != end)
	{
		if (POLICY::eq(*itr, std::forward<VALUE>(value))) return itr;

		++itr;
	}

	return null;
}

template <class POLICY, class TYPE> inline Idx SearchRegion(const ArrayView <TYPE> & haystack, const ArrayView <TYPE> & needle)
{
	REFLEX_ASSERT(needle.size);

	if (needle.size <= haystack.size)
	{
		ArrayView <TYPE> test(haystack.data, needle.size);

		auto end = haystack.data + (haystack.size - needle.size) + 1;

		while (test.data < end)
		{
			if (POLICY::eq(test, needle)) return UInt(test.data - haystack.data);

			test.data++;
		}
	}

	return {};
}

template <class POLICY, class TYPE> inline Idx ReverseSearchRegion(const ArrayView <TYPE> & haystack, const ArrayView <TYPE> & needle)
{
	REFLEX_ASSERT(needle.size);

	if (needle.size <= haystack.size)
	{
		ArrayView <TYPE> test(haystack.data + (haystack.size - needle.size), needle.size);

		do
		{
			if (POLICY::eq(test, needle)) return UInt(test.data - haystack.data);
		}
		while (test.data-- != haystack.data);
	}

	return {};
}

template <class POLICY, class TYPE> inline void ReplaceElement(Array <TYPE> & value, const TYPE & from, const TYPE & to)
{
	for (auto & i : value)
	{
		if (POLICY::eq(i, from)) i = to;
	}
}

template <class POLICY, class TYPE> Array <TYPE> ReplaceRegion(const ArrayView <TYPE> & value, const ArrayView <TYPE> & from, const ArrayView <TYPE> & to);

template <class CASEPOLICY, class TYPE> REFLEX_INLINE void RemoveRegion(Array <TYPE> & array, ArrayView <TYPE> value)
{
	REFLEX_ASSERT(value.size != 0);

	auto a0 = ToUIntNative(array.GetData());
	auto a1 = a0 + UIntNative(array.GetSize() * sizeof(TYPE));

	auto v0 = ToUIntNative(value.data);
	auto v1 = v0 + UIntNative(value.size * sizeof(TYPE));

	REFLEX_ASSERT(!((v0 < a1) && (v1 > a0)));

	UInt position = 0;

	while (position < array.GetSize())
	{
		auto view = ToView(array);

		if (auto idx = SearchRegion<CASEPOLICY>(Mid(view, position), value))
		{
			position += idx.value;

			array.Remove(position, value.size);
		}
		else
		{
			break;
		}
	}
}

template <class POLICY, class TYPE, class VALUE> inline void RemoveValue(Array <TYPE> & array, VALUE && value)
{
	if constexpr (kIsType<TYPE, VALUE>)
	{
		REFLEX_ASSERT(!Inside<UIntNative>(ToUIntNative(GetAdr(value)), ToUIntNative(array.GetData()), array.GetSize() * sizeof(TYPE)));
	}

	auto start = array.GetData();

	auto ptr = start + array.GetSize();

	while (ptr-- != start)
	{
		if (POLICY::eq(*ptr, std::forward<VALUE>(value)))
		{
			array.Remove(UInt(ptr - start));
		}
	}
}

REFLEX_END

template <class POLICY, class ARRAY, class VALUE> REFLEX_INLINE Reflex::Idx Reflex::Search(const ARRAY & array, VALUE && value)
{
	auto view = ToView(array);

	using ArrayViewType = decltype(view);

	if constexpr (std::is_convertible_v<VALUE, ArrayViewType>)
	{
		return Detail::SearchRegion<POLICY>(view, ToView(value));
	}
	else
	{
		return Detail::SearchElement<POLICY>(view, std::forward<VALUE>(value));
	}
}

template <class POLICY, class ARRAY, class VALUE> REFLEX_INLINE Reflex::Idx Reflex::ReverseSearch(const ARRAY & array, VALUE && value)
{
	auto view = ToView(array);

	using ArrayViewType = decltype(view);

	if constexpr (std::is_convertible_v<VALUE, ArrayViewType>)
	{
		return Detail::ReverseSearchRegion<POLICY>(view, ToView(value));
	}
	else
	{
		return Detail::ReverseSearchElement<POLICY>(view, std::forward<VALUE>(value));
	}
}

template <class POLICY, class VALUE, class ARRAY> REFLEX_INLINE Reflex::Detail::ArrayItemType <ARRAY> * Reflex::SearchValue(ARRAY & haystack, VALUE && value, Detail::ArrayItemType <ARRAY> * null)
{
	return Detail::SearchValue<POLICY>(ToRegion(haystack), value, null);
}

template <class POLICY, class VALUE, class ARRAY> REFLEX_INLINE Reflex::Detail::ArrayItemType <const ARRAY> * Reflex::SearchValue(const ARRAY & haystack, VALUE && value, Detail::ArrayItemType <const ARRAY> * null)
{
	return Detail::SearchValue<POLICY>(ToView(haystack), value, null);
}

template <class POLICY, class ARRAY, class FROM, class TO> inline auto Reflex::Replace(const ARRAY & array, const FROM & from, const TO & to)
{
	auto array_view = ToView(array);

	using ArrayViewType = decltype(array_view);

	if constexpr (std::is_convertible<FROM,ArrayViewType>::value && std::is_convertible<TO,ArrayViewType>::value)
	{
		return Detail::ReplaceRegion<POLICY>(array_view, ToView(from), ToView(to));
	}
	else
	{
		REFLEX_STATIC_ASSERT((!std::is_convertible<FROM,ArrayViewType>::value || std::is_convertible<TO,ArrayViewType>::value));

		Array rtn = array_view;

		Detail::ReplaceElement<POLICY>(rtn, from, to);

		return rtn;
	}
}

template <class POLICY, class TYPE, class VALUE> inline void Reflex::RemoveValue(Array <TYPE> & array, VALUE && value)
{
	Detail::RemoveValue<POLICY, TYPE, VALUE>(array, std::forward<VALUE>(value));
}

template <class POLICY, class TYPE, class VALUE> REFLEX_INLINE void Reflex::Remove(Array <TYPE> & array, VALUE && element_or_array)
{
	if constexpr (std::is_convertible< VALUE, ArrayView <TYPE> >::value)
	{
		Detail::RemoveRegion<POLICY>(array, ToView(element_or_array));
	}
	else
	{
		Detail::RemoveValue<POLICY,TYPE,VALUE>(array, std::forward<VALUE>(element_or_array));
	}
}

template <class POLICY, class ARRAY, class VALUE> REFLEX_INLINE auto Reflex::Filter(const ARRAY & array, VALUE && element_or_array)
{
	Array rtn = ToView(array);

	Remove(rtn, std::forward<VALUE>(element_or_array));

	return rtn;
}

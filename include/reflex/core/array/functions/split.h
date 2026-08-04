#pragma once

#include "join.h"




//
//Primary API

namespace Reflex
{

	template <class ARRAY, class DELIMITER> auto Split(const ARRAY & arr, const DELIMITER & delimiter_element_or_array);

	template <class ARRAY, class DELIMITER> auto Merge(const ARRAY & values, const DELIMITER & delimiter);

}




//
//impl

REFLEX_NS(Reflex::Detail)

template <class TYPE, class OUTPUT> void Split(const ArrayView <TYPE> & view, const TYPE & delimiter, OUTPUT & output)
{
	auto prev = view.data;

	auto end = view.data + view.size;

	REFLEX_LOOP_PTR(view.data, ptr, view.size)
	{
		if (*ptr == delimiter)
		{
			output.Push(ArrayView<TYPE>(prev, UInt(ptr - prev)));

			prev = ptr + 1;
		}
	}

	output.Push(Right<false>(view, UInt(end - prev)));
}

template <class TYPE, class OUTPUT> void SplitArray(const ArrayView <TYPE> & view, const ArrayView <TYPE> & delimiter, OUTPUT & output)
{
	auto itr = view;

	while (auto idx = SearchRegion<StandardCompare>(itr, delimiter))
	{
		auto section = Left(itr, idx.value);

		output.Push(section);

		itr = Mid(itr, idx.value + delimiter.size);
	}

	output.Push(itr);
}

template <class ARRAY, class TYPE> inline Array <TYPE> Merge(const ArrayView <ARRAY> & values, const ArrayView <TYPE> & delimiter)
{
	Array <TYPE> rtn;

	if (values)
	{
		for (auto & i : values) rtn = Join(rtn, i, delimiter);

		rtn.Shrink(delimiter.size);
	}

	return rtn;
}

REFLEX_END

template <class ARRAY, class VALUE> inline auto Reflex::Split(const ARRAY & array, const VALUE & delimiter)
{
	auto view = ToView(array);

	using ArrayViewType = decltype(view);

	Array <ArrayViewType> rtn;

	if constexpr (std::is_convertible_v<VALUE, ArrayViewType>)
	{
		Detail::SplitArray(view, ToView(delimiter), rtn);
	}
	else
	{
		Detail::Split<typename ArrayViewType::Type>(view, delimiter, rtn);
	}

	return rtn;
}

template <class ARRAY, class DELIMITER> inline auto Reflex::Merge(const ARRAY & values, const DELIMITER & delimiter)
{
	return Detail::Merge(ToView(values), ToView(delimiter));
}

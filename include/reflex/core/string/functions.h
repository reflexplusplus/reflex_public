#pragma once

#include "[require].h"




//
//Secondary API

namespace Reflex
{

	template <class CHARACTER> constexpr bool IsWhiteSpace(CHARACTER c) { return static_cast<UInt8>(c) < 33; }

	template <auto CHARACTER> constexpr bool IsCharacter(decltype(CHARACTER) c) { return c == CHARACTER; }


	template <class CHARACTER> ArrayView <CHARACTER> Trim(const ArrayView <CHARACTER> & view, bool(*fn)(NonConstT<CHARACTER>) = &IsWhiteSpace<NonConstT<CHARACTER>>);

	template <class CHARACTER> ArrayView <CHARACTER> TrimLeft(const ArrayView <CHARACTER> & view, bool(*fn)(NonConstT<CHARACTER>) = &IsWhiteSpace<NonConstT<CHARACTER>>);

	template <class CHARACTER> ArrayView <CHARACTER> TrimRight(const ArrayView <CHARACTER> & view, bool(*fn)(NonConstT<CHARACTER>) = &IsWhiteSpace<NonConstT<CHARACTER>>);


	template <class CHARACTER, class X> inline void operator+(const ArrayView <CHARACTER> & a, const X & b);	//intentionally not implemented, use Join

	template <class CHARACTER, class X> inline void operator+(const Array <CHARACTER> & a, const X & b);		//intentionally not implemented, use Join

}




//
//impl

REFLEX_NS(Reflex::Detail)

template <class CHARACTER> inline Array <CHARACTER> PadLeft(const ArrayView <CHARACTER> & view, UInt min_length, CHARACTER c)
{
	if (view.size < min_length)
	{
		Array <CHARACTER> t(min_length);

		auto n = min_length - view.size;

		Fill({ t.GetData(), n }, c);

		Copy({ t.GetData() + n, view.size }, view.data);

		return t;
	}
	else
	{
		return view;
	}
}

REFLEX_END

template <class CHARACTER> REFLEX_INLINE Reflex::ArrayView <CHARACTER> Reflex::Trim(const ArrayView <CHARACTER> & string, bool(*test)(NonConstT<CHARACTER>))
{
	return TrimRight(TrimLeft(string, test), test);
}

template <class CHARACTER> REFLEX_INLINE Reflex::ArrayView <CHARACTER> Reflex::TrimLeft(const ArrayView <CHARACTER> & string, bool(*test)(NonConstT<CHARACTER>))
{
	auto rtn = string;

	while (rtn.size)
	{
		if (test(*rtn.data))
		{
			rtn.data++;

			rtn.size--;
		}
		else
		{
			break;
		}
	}

	return rtn;
}

template <class CHARACTER> inline Reflex::ArrayView <CHARACTER> Reflex::TrimRight(const ArrayView <CHARACTER> & string, bool(*test)(NonConstT<CHARACTER>))
{
	auto end = string.data + string.size;

	while (end > string.data)
	{
		if (test(end[-1]))
		{
			end--;
		}
		else
		{
			break;
		}
	}

	return { string.data, UInt(end - string.data) };
}

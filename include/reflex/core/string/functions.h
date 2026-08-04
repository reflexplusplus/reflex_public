#pragma once

#include "defines.h"




//
//Secondary API

namespace Reflex
{

	template <class CHARACTER> constexpr bool IsWhiteSpace(CHARACTER c) { return static_cast<UInt8>(c) < 33; }


	CString::View TrimLeft(CString::View view, FunctionPointer <bool(char)> = &IsWhiteSpace<char>);

	CString::View TrimRight(CString::View view, FunctionPointer <bool(char)> = &IsWhiteSpace<char>);

	CString::View Trim(CString::View view, FunctionPointer <bool(char)> = &IsWhiteSpace<char>);


	WString::View TrimLeft(WString::View view, FunctionPointer <bool(WChar)> = &IsWhiteSpace<WChar>);

	WString::View TrimRight(WString::View view, FunctionPointer <bool(WChar)> = &IsWhiteSpace<WChar>);

	WString::View Trim(WString::View view, FunctionPointer <bool(WChar)> = &IsWhiteSpace<WChar>);



	CString::View TrimLeft(CString && view, FunctionPointer <bool(char)> = &IsWhiteSpace<char>) = delete;

	CString::View TrimRight(CString && view, FunctionPointer <bool(char)> = &IsWhiteSpace<char>) = delete;

	CString::View Trim(CString && view, FunctionPointer <bool(char)> = &IsWhiteSpace<char>) = delete;;


	WString::View TrimLeft(WString && view, FunctionPointer <bool(WChar)> = &IsWhiteSpace<WChar>) = delete;

	WString::View TrimRight(WString && view, FunctionPointer <bool(WChar)> = &IsWhiteSpace<WChar>) = delete;

	WString::View Trim(WString && view, FunctionPointer <bool(WChar)> = &IsWhiteSpace<WChar>) = delete;


	template <class CHARACTER, class X> inline void operator+(ArrayView <CHARACTER> a, const X & b) = delete;	//use Join

	template <class CHARACTER, class X> inline void operator+(const Array <CHARACTER> & a, const X & b) = delete;		//use Join

}




//
//impl

REFLEX_NS(Reflex::Detail)

template <class CHARACTER> inline Array <NonConstT<CHARACTER>> PadLeft(ArrayView <CHARACTER> view, UInt min_length, CHARACTER c)
{
	if (view.size < min_length)
	{
		Array <NonConstT<CHARACTER>> t(min_length);

		auto n = min_length - view.size;

		Fill({ t.GetData(), n }, c);

		Copy(view, { t.GetData() + n, view.size });

		return t;
	}
	else
	{
		return view;
	}
}

template <class CHARACTER> REFLEX_INLINE ArrayRegion <CHARACTER> TrimLeft(ArrayRegion <CHARACTER> string, bool(*test)(NonConstT<CHARACTER>))
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

template <class CHARACTER> inline ArrayRegion <CHARACTER> TrimRight(ArrayRegion <CHARACTER> string, bool(*test)(NonConstT<CHARACTER>))
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

template <class CHARACTER> REFLEX_INLINE ArrayRegion <CHARACTER> Trim(ArrayRegion <CHARACTER> string, bool(*test)(NonConstT<CHARACTER>))
{
	return TrimRight<CHARACTER>(TrimLeft<CHARACTER>(string, test), test);
}

REFLEX_END

REFLEX_INLINE Reflex::CString::View Reflex::Trim(CString::View string, FunctionPointer <bool(char)> test)
{
	return Detail::Trim<const char>(string, test);
}

REFLEX_INLINE Reflex::CString::View Reflex::TrimLeft(CString::View string, FunctionPointer <bool(char)> test)
{
	return Detail::TrimLeft<const char>(string, test);
}

REFLEX_INLINE Reflex::CString::View Reflex::TrimRight(CString::View string, FunctionPointer <bool(char)> test)
{
	return Detail::TrimRight<const char>(string, test);
}

REFLEX_INLINE Reflex::WString::View Reflex::Trim(WString::View string, FunctionPointer <bool(WChar)> test)
{
	return Detail::Trim<const WChar>(string, test);
}

REFLEX_INLINE Reflex::WString::View Reflex::TrimLeft(WString::View string, FunctionPointer <bool(WChar)> test)
{
	return Detail::TrimLeft<const WChar>(string, test);
}

REFLEX_INLINE Reflex::WString::View Reflex::TrimRight(WString::View string, FunctionPointer <bool(WChar)> test)
{
	return Detail::TrimRight<const WChar>(string, test);
}

#pragma once

#include "[require].h"




//
//Primary API

namespace Reflex
{

	template <class CHARACTER> using String = Array <CHARACTER>;

	using CString = String <char>;

	using CString16 = String <char16_t>;

	using WString = String <WChar>;

}

REFLEX_EXTERN_NULL(Reflex::CString);
REFLEX_EXTERN_NULL(Reflex::WString);
REFLEX_EXTERN_NULL(Reflex::ObjectOf<CString>);
REFLEX_EXTERN_NULL(Reflex::ObjectOf<WString>);

REFLEX_INSTANTIATE_TYPEID(Reflex::ObjectOf<CString>);
REFLEX_INSTANTIATE_TYPEID(Reflex::ObjectOf<WString>);

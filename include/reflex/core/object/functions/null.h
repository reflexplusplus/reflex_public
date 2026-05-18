#pragma once

#include "../detail/nullable.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE> TRef <TYPE> Null();


}




//
//Detail

namespace Reflex::Detail
{
	
	template <class TYPE> constexpr bool kIsNullable = kIsType< NonRefT<decltype(GetNullInstance<TYPE>())>, TYPE > || InheritsFrom< NonRefT<decltype(GetNullInstance<TYPE>())>, TYPE >::value;

}




//
//impl

template <class TYPE> inline Reflex::TRef <TYPE> Reflex::Null()
{
	return Detail::GetNullInstance<NonConstT<TYPE>>();
}

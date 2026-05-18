#pragma once

#include "[require].h"




//
//Primary API

namespace Reflex::Async
{

	TRef <Object> CreateClock(const Function <void()> & callback);

	template <class auto_1, class CALLBACK> inline TRef <Object> CreateClock(auto_1 && ptr_or_ref, CALLBACK && callback);

}




//
//impl

template <class auto_1, class CALLBACK> inline Reflex::TRef <Reflex::Object> Reflex::Async::CreateClock(auto_1 && tref, CALLBACK && callback)
{
	auto & ref = Deref(tref);

	FunctionPointer <void(decltype(ref))> confirm_castable = callback;

	return System::CreateListener(System::kNotificationClock, &ref, reinterpret_cast<FunctionPointer<void(void *)>>(confirm_castable));
}

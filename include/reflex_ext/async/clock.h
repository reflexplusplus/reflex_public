#pragma once

#include "[require].h"




//
//Primary API

namespace Reflex::Async
{

	[[nodiscard]] TRef <Object> CreateClock(const Function <void()> & callback);

	template <class auto_1, class CALLABLE> [[nodiscard]] TRef <Object> CreateClock(auto_1 && ptr_or_ref, CALLABLE && callback);

}




//
//impl

template <class auto_1, class CALLABLE> inline Reflex::TRef <Reflex::Object> Reflex::Async::CreateClock(auto_1 && tref, CALLABLE && callback)
{
	auto & ref = Deref(tref);

	auto typed = FunctionPointer<void(decltype(ref)&)>(callback);
	auto downcast = Reflex::Detail::CastFunctionPointer<FunctionPointer <void(void *)>>(typed);

	return System::CreateListener(System::kNotificationClock, &ref, downcast);
}

#pragma once

#include "reflex/glx.h"




//
//Addon API

namespace Reflex::GLX
{

	template <class TYPE> auto Init(TYPE && objectref, const Style & style);

	template <class TYPE> auto Init(TYPE && objectref, const Style & style, WString && label);

	template <class TYPE> auto Init(TYPE && objectref, const Style & style, const WString & label);

}




//
//imp

template <class TYPE> REFLEX_INLINE auto Reflex::GLX::Init(TYPE && objectref, const Style & style)
{
	auto & object = Deref(objectref);

	object.SetStyle(style);

	return TRef<NonRefT<decltype(object)>>(object);
}

template <class TYPE> REFLEX_INLINE auto Reflex::GLX::Init(TYPE && objectref, const Style & style, WString && label)
{
	auto & object = Deref(objectref);

	object.SetStyle(style);

	SetText(object, std::move(label));

	return TRef<NonRefT<decltype(object)>>(object);
}

template <class TYPE> inline auto Reflex::GLX::Init(TYPE && objectref, const Style & style, const WString & label)
{
	return Init(objectref, style, Copy(label));
}

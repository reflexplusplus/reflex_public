#pragma once

#include "reflex/glx.h"




//
//Addon API

namespace Reflex::GLX
{

	void SetOnStyle(Object & object, Key32 delegate_id, const Function <void(const Style&)> & callback);
	
	void UnsetOnStyle(Object & object, Key32 delegate_id);


	void SelectChildren(Object & object, bool select = true);

	void SelectBranch(Object & object, bool select = true);


	void ActivateBranch(Object & object, bool enable = true);


	void SetBranchState(Object & object, Key32 state);

	void ClearBranchState(Object & object, Key32 state);

}




//
//impl

REFLEX_NS(Reflex::GLX)
[[deprecated("use SetOnStyle(obj,delegate_id,fn)")]] inline void SetOnStyle(Object & object, const Function <void(const Style&)> &callback)
{
	return SetOnStyle(object, MakeKey32("GLX::SetOnStyle"), callback);
}
//will be deprecated
template <class TYPE> auto Init(TYPE && objectref, const Style & style);
template <class TYPE> auto Init(TYPE && objectref, const Style & style, WString && label);
template <class TYPE> auto Init(TYPE && objectref, const Style & style, const WString & label);
REFLEX_END

inline void Reflex::GLX::SelectBranch(Object & object, bool select)
{
	if (select)
	{
		SetBranchState(object, kSelectedState);
	}
	else
	{
		ClearBranchState(object, kSelectedState);
	}
}

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

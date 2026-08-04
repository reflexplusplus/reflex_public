#pragma once

#include "input.h"




//
//Primary API

namespace Reflex::GLX
{

	void SetState(Object & object, Key32 state, bool value = true);

	bool ToggleState(Object & object, Key32 state);


	void Activate(Object & object, bool enabled = true);

	bool IsActive(const Object & object);


	void Select(Object & object, bool select = true);

	bool IsSelected(const Object & object);

}




//
//impl

inline void Reflex::GLX::SetState(Object & object, Key32 state, bool value)
{
	if (value)
	{
		object.SetState(state);
	}
	else
	{
		object.ClearState(state);
	}
}

inline bool Reflex::GLX::ToggleState(Object & object, Key32 state)
{
	if (object.CheckState(state))
	{
		object.ClearState(state);

		return false;
	}
	else
	{
		object.SetState(state);

		return true;
	}
}

inline void Reflex::GLX::Activate(Object & object, bool enable)
{
	EnableMouse(object, enable);

	SetState(object, kInactiveState, !enable);
}

inline bool Reflex::GLX::IsActive(const Object & object)
{
	return !object.CheckState(kInactiveState);
}

inline void Reflex::GLX::Select(Object & object, bool select)
{
	SetState(object, kSelectedState, select);
}

inline bool Reflex::GLX::IsSelected(const Object & object)
{
	return object.CheckState(kSelectedState);
}

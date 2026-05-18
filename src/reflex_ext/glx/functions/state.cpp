#include "../../../../include/reflex_ext/glx/functions/state.h"




void Reflex::GLX::SelectChildren(Object & object, bool select)
{
	if (select)
	{
		for (auto & i : object) i.SetState(kSelectedState);
	}
	else
	{
		for (auto & i : object) i.ClearState(kSelectedState);
	}
}

void Reflex::GLX::ActivateBranch(Object & object, bool enable)
{
	if (enable)
	{ 
		EnableMouse(object, true, false);

		ClearBranchState(object, kInactiveState);
	}
	else
	{
		EnableMouse(object, false, true);

		SetBranchState(object, kInactiveState);
	}
}

void Reflex::GLX::SetBranchState(Object & object, Key32 state)
{
	object.SetState(state);

	for (auto & i : object)
	{
		SetBranchState(i, state);
	}
}

void Reflex::GLX::ClearBranchState(Object & object, Key32 state)
{
	object.ClearState(state);

	for (auto & i : object)
	{
		ClearBranchState(i, state);
	}
}

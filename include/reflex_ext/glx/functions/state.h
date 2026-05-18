#pragma once

#include "reflex/glx.h"




//
//Addon API

namespace Reflex::GLX
{

	void SelectChildren(Object & object, bool select = true);

	void SelectBranch(Object & object, bool select = true);


	void ActivateBranch(Object & object, bool enable = true);


	void SetBranchState(Object & object, Key32 state);

	void ClearBranchState(Object & object, Key32 state);

}




//
//impl

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

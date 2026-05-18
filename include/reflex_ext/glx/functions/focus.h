#pragma once

#include "reflex/glx.h"




//
//Addon API

namespace Reflex::GLX
{

	void FocusBranch(Object & branch_root);							//focus 'branch_root' if focus not already within 'branch_root'

	void RedirectFocus(Object & branch_root, Object & object);		//focus 'object' if focus is within 'branch_root'


	REFLEX_DECLARE_KEY32(WantsFocus);			//Use Data::SetBool(object, kWantsFocus, true)

	void EnableFocusCycle(Object & root);		//typically set at branch root to allow on all objects within branch


	void EnableFocusHighlight(Object & root);	//typically set at branch root to allow on all objects within branch

	void DisableFocusHighlight(Object & root);


	void SetFocusHighlight(Object & scope, const Function <TRef<Animation>()> & ctr);	//typically set at root, is inherited

	void ClearFocusHighlight(Object & scope);

}

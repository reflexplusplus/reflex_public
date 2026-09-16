#pragma once

#include "enter_exit.h"




//
//Addon API

namespace Reflex::GLX
{

	AlreadyRetained <Object> Acquire(Object & parent, Key32 id, UInt8 enter_animation_flags, const Function <Unretained<Object>()> & ctr);

	void Discard(Object & parent, Key32 id);



	//
	//overlay

	AlreadyRetained <Object> AcquireOverlay(Object & parent, Key32 id, bool block_input, bool block_dragdrop, Key32 style_id, const Function <void(Object & overlay)> & oninit);

	bool DiscardOverlay(Object & parent, Key32 id);

}




//
//Detail

namespace Reflex::GLX::Detail
{

	bool DiscardOverlay(Object & overlay);

}

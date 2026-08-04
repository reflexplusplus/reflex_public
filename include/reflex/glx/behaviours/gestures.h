#pragma once

#include "../object.h"




//
//Experimental API

namespace Reflex::GLX
{

	REFLEX_GLX_EVENT_ID(TapGesture);

	REFLEX_GLX_EVENT_ID(LongTapGesture);

	TRef <Object::Delegate> CreateTapGestureRecognizer();


	REFLEX_GLX_EVENT_ID(PanGesture);

	TRef <Object::Delegate> CreatePanOrTapGestureRecognizer();


	REFLEX_GLX_EVENT_ID(SwipeGesture);

	TRef <Object::Delegate> CreateSwipeOrTapGestureRecognizer();

}

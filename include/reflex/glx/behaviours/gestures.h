#pragma once

#include "detail/gestures.h"




//
//Experimental API

namespace Reflex::GLX
{

	REFLEX_GLX_EVENT_ID(LongTapGesture);

	TRef <Object::Delegate> CreateLongTapGestureRecognizer(bool emulate_pointer_down);


	constexpr Float32 kDefaultTouchMoveThreshold = 4.0f;

	REFLEX_GLX_EVENT_ID(PanGesture);

	TRef <Object::Delegate> CreatePanGestureRecognizer(bool emulate_pointer_down, Float32 threshold = kDefaultTouchMoveThreshold);


	REFLEX_GLX_EVENT_ID(SwipeGesture);

	TRef <Object::Delegate> CreateSwipeGestureRecognizer(bool emulate_pointer_down, Float32 threshold = kDefaultTouchMoveThreshold);


	void IgnoreGestures(Object & object, bool include_children = true);

}

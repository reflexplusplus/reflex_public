#pragma once

#include "detail/gestures.h"




//
//Experimental API

namespace Reflex::GLX
{

	REFLEX_GLX_EVENT_ID(LongTapGesture);

	[[nodiscard]] Unretained <Object::Delegate> CreateLongTapGestureRecognizer(bool emulate_pointer_down);


	constexpr Float32 kDefaultTouchMoveThreshold = 4.0f;

	REFLEX_GLX_EVENT_ID(PanGesture);

	[[nodiscard]] Unretained <Object::Delegate> CreatePanGestureRecognizer(bool emulate_pointer_down, Float32 threshold = kDefaultTouchMoveThreshold);


	REFLEX_GLX_EVENT_ID(SwipeGesture);

	[[nodiscard]] Unretained <Object::Delegate> CreateSwipeGestureRecognizer(bool emulate_pointer_down, Float32 threshold = kDefaultTouchMoveThreshold);


	void IgnoreGestures(Object & object, bool include_children = true);

}

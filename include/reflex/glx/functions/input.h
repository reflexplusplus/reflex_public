#pragma once

#include "../window.h"
#include "lookup.h"




//
//Primary API

namespace Reflex::GLX
{

	void EnableMouse(Object & object, bool enable = true, bool intercept = false);


	Point GetPointerPosition(const WindowClient & window);


	Point GetPointerPosition(const Object & object, const Event & e);

	[[deprecated ("user GetPointerPosition")]] Point GetMousePosition(const Object & object);


	bool ExceedsDragThreshold(Point drag, Float sens = 2.0f);


	Point TransformPosition(const Object & object, Point window_coordinates_position);

	Point ScaleDelta(const Object & object, Point window_coordinates_delta);

}




//
//impl

REFLEX_NS(Reflex::GLX::Detail)

inline const Core::Pointer * QueryPointer(const WindowClient & window)
{
	auto pointers = Core::desktop->GetPointers();

	for (auto & i : pointers)
	{
		if (i.window == window)
		{
			return &i;
		}
	}

	return nullptr;
}

inline const Core::Pointer & GetActivePointer()
{
	auto pointers = Core::desktop->GetPointers();

	for (auto & i : ReverseIterate(pointers))
	{
		if (i.pressed) return i;
	}

	return pointers.GetFirst();
}

REFLEX_END

REFLEX_INLINE void Reflex::GLX::EnableMouse(Object & object, bool enable, bool intercept)
{
	object.EnablePointer(enable, intercept);
}

inline Reflex::GLX::Point Reflex::GLX::GetPointerPosition(const WindowClient & window)
{
	auto pointers = Core::desktop->GetPointers();

	for (auto & i : pointers)
	{
		if (i.window == window)
		{
			return i.position;
		}
	}

	auto & pointer = pointers.GetLast();

	return pointer.position + pointer.window->GetRect().origin - window.GetRect().origin;
}

inline Reflex::GLX::Point Reflex::GLX::GetPointerPosition(const Object & object, const Event & e)
{
	return TransformPosition(object, GetPosition(e));
}

inline Reflex::GLX::Point Reflex::GLX::GetMousePosition(const Object & object)
{
	return TransformPosition(object, Detail::GetActivePointer().position);
}

inline Reflex::GLX::Point Reflex::GLX::TransformPosition(const Object & object, Point absolute_position)
{
	auto [position, scale] = CalculateAbs(object);

	return (absolute_position - position) / scale;
}

inline Reflex::GLX::Point Reflex::GLX::ScaleDelta(const Object & object, Point delta)
{
	return delta / CalculateAbs(object).b;
}

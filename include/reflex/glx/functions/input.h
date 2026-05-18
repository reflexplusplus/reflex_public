#pragma once

#include "lookup.h"
#include "../event/defines.h"




//
//

namespace Reflex::GLX
{

	void EnableMouse(Object & object, bool enable = true, bool intercept = false);

	Pair <bool> MouseEnabled(const Object & object);


	void EnableMouseCapture(Object & object, bool enable = true, bool incremental = false);


	bool ExceedsDragThreshold(Point drag, Float sens = 2.0f);


	Point TransformPosition(const Object & object, Point window_coordinates_position);

	Point ScaleDelta(const Object & object, Point window_coordinates_delta);

	Point GetMousePosition(const Object & object);

}




//
//impl

REFLEX_INLINE void Reflex::GLX::EnableMouse(Object & object, bool enable, bool intercept)
{
	constexpr Trap kMap[2][2] = { { kTrapThru, kTrapReject }, { kTrapPassive, kTrapActive } };

	object.SetMouseOverTrapMode(kMap[enable][intercept]);

#if REFLEX_DEBUG
	auto test = enable ? (intercept ? kTrapActive : kTrapPassive) : (intercept ? kTrapReject : kTrapThru);

	constexpr GLX::Trap kMouseOverReturn[4] =
	{
		kTrapThru,
		kTrapPassive,
		kTrapReject,
		kTrapActive
	};

	REFLEX_ASSERT(test == kMouseOverReturn[MakeBits(enable, intercept)]);

	REFLEX_ASSERT(object.GetMouseOverTrapMode() == test);
#endif
}

REFLEX_INLINE void Reflex::GLX::EnableMouseCapture(Object & object, bool enable, bool incremental)
{
	constexpr Trap kMap[2][2] = { { kTrapPassive, kTrapPassive }, { kTrapActive, kTrapActiveIncremental } };

	object.SetMouseClickTrapMode(kMap[enable][incremental]);

#if REFLEX_DEBUG
	auto test = enable ? (incremental ? kTrapActiveIncremental : kTrapActive) : kTrapPassive;

	REFLEX_ASSERT(object.GetMouseClickTrapMode() == test);
#endif
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

inline Reflex::GLX::Point Reflex::GLX::GetMousePosition(const Object & object)
{
	return TransformPosition(object, object.GetWindow()->GetMousePosition());
}

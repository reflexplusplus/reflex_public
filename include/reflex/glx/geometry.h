#pragma once

#include "core.h"




//
//Primary API

namespace Reflex::GLX
{

	using Point = System::fPoint;

	using ColourPoint = System::ColourPoint;

	using Size = System::fSize;

	using Scale = System::fSize;

	using Rect = System::fRect;

	struct Margin;

	struct Range;


	bool operator==(const Range & a, const Range & b);

}




//
//Margin

struct Reflex::GLX::Margin
{
	Size near, far;
};

REFLEX_ASSERT_RAW(Reflex::GLX::Margin);




//
//Range

struct Reflex::GLX::Range
{
	Float start = 0.0f;
	Float length = 0.0f;
};

REFLEX_ASSERT_RAW(Reflex::GLX::Range);




//
//impl

REFLEX_INLINE bool Reflex::GLX::operator==(const Range & a, const Range & b)
{
	return Reinterpret<Size>(a) == Reinterpret<Size>(b);
}

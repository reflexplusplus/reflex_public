#pragma once

#include "../[require].h"




//
//Primary API

namespace Reflex::GLX
{

	constexpr Colour RGB(UInt8 grey);

	constexpr Colour RGB(UInt8 grey, UInt8 alpha);

	constexpr Colour RGB(UInt8 red, UInt8 green, UInt8 blue);

	constexpr Colour RGB(UInt8 red, UInt8 green, UInt8 blue, UInt8 alpha);

}




//
//Reflex:: overloads

namespace Reflex
{

	GLX::Colour LinearInterpolate(Float x, const GLX::Colour & a, const GLX::Colour & b);

}




//
//impl

REFLEX_INLINE Reflex::GLX::Colour Reflex::LinearInterpolate(Float x, const GLX::Colour & a, const GLX::Colour & b)
{
	Float dry = 1.0f - x;

	return { (a.r * dry) + (b.r * x), (a.g * dry) + (b.g * x), (a.b * dry) + (b.b * x), (a.a * dry) + (b.a * x) };
}

REFLEX_INLINE constexpr Reflex::GLX::Colour Reflex::GLX::RGB(UInt8 grey)
{
	Float value = grey / 255.0f;

	return { value, value, value, 1.0f };
}

REFLEX_INLINE constexpr Reflex::GLX::Colour Reflex::GLX::RGB(UInt8 grey, UInt8 alpha)
{
	Float value = grey / 255.0f;

	return { value, value, value, alpha / 255.0f };
}

REFLEX_INLINE constexpr Reflex::GLX::Colour Reflex::GLX::RGB(UInt8 red, UInt8 green, UInt8 blue)
{
	return { red / 255.0f, green / 255.0f, blue / 255.0f, 1.0f };
}

REFLEX_INLINE constexpr Reflex::GLX::Colour Reflex::GLX::RGB(UInt8 red, UInt8 green, UInt8 blue, UInt8 alpha)
{
	return { red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f };
}

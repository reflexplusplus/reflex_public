#pragma once

#include "../defines.h"




//
//Primary API

namespace Reflex::GLX
{

	constexpr Margin MakeMargin(Float size);

	constexpr Margin MakeMargin(Float w, Float h);

	constexpr Margin MakeMargin(Float left, Float top, Float right, Float bottom);


	Float Area(Size size);


	bool Contains(Size size, Point point);

	bool Contains(const Rect & rect, Point point);

	bool Contains(const Rect & rect, const Rect & inner);

	bool CircleContains(Float diameter, Point point);


	bool Intersects(const Rect & a, const Rect & b);


	Rect Rectify(const Rect & rect);


	Rect Indent(Size size, const Margin & amount);


	Rect Indent(const Rect & rect, Float amount);

	Rect Indent(const Rect & rect, Size amount);

	Rect Indent(const Rect & rect, const Margin & amount);


	Rect Expand(const Rect & rect, Float amount);

	Rect Expand(const Rect & rect, Size amount);

	Rect Expand(const Rect & rect, const Margin & amount);


	Size Sum(const Margin & margin);


	inline Float ClipValue(Float value, Range range)
	{
		return Clip(value, range.start, range.start + range.length);
	}

	inline Float NormaliseValue(Float value, Range range)
	{
		if (range.length)
		{
			return Clip((value - range.start) / range.length, 0.0f, 1.0f);
		}
		else
		{
			return 0.0f;
		}
	}

}




//
//Reflex:: overloads

namespace Reflex
{

	GLX::Size Reciprocal(GLX::Size scale);

	GLX::Size Abs(GLX::Size size);

	GLX::Point LinearInterpolate(Float x, GLX::Point a, GLX::Point b);

	GLX::Size LinearInterpolate(Float x, GLX::Size a, GLX::Size b);

}




//
//impl

REFLEX_INLINE constexpr Reflex::GLX::Margin Reflex::GLX::MakeMargin(Float size)
{
	return { { size, size }, { size, size } };
}

REFLEX_INLINE constexpr Reflex::GLX::Margin Reflex::GLX::MakeMargin(Float w, Float h)
{
	return { { w, h }, { w, h } };
}

REFLEX_INLINE constexpr Reflex::GLX::Margin Reflex::GLX::MakeMargin(Float left, Float top, Float right, Float bottom)
{
	return { { left, top }, { right, bottom } };
}

REFLEX_INLINE Reflex::GLX::Size Reflex::Abs(GLX::Size size)
{
	return { Abs(size.w), Abs(size.h) };
}

REFLEX_INLINE Reflex::GLX::Point Reflex::LinearInterpolate(Float x, GLX::Point a, GLX::Point b)
{
	return { a.x + ((b.x - a.x) * x), a.y + ((b.y - a.y) * x) };
}

REFLEX_INLINE Reflex::GLX::Size Reflex::LinearInterpolate(Float x, GLX::Size a, GLX::Size b)
{
	return { a.w + ((b.w - a.w) * x), a.h + ((b.h - a.h) * x) };
}

REFLEX_INLINE Reflex::Float Reflex::GLX::Area(Size size)
{
	return size.w * size.h;
}

REFLEX_INLINE Reflex::GLX::Size Reflex::Reciprocal(GLX::Size scale)
{
	return { 1.0f / scale.w, 1.0f / scale.h };
}

REFLEX_INLINE bool Reflex::GLX::Intersects(const Rect & a, const Rect & b)
{
	auto [ax1, ay1] = a.origin;
	auto ax2 = ax1 + a.size.w;
	auto ay2 = ay1 + a.size.h;

	auto [bx1, by1] = b.origin;
	auto bx2 = bx1 + b.size.w;
	auto by2 = by1 + b.size.h;

	return !(ax1 > bx2 || bx1 > ax2 || ay1 > by2 || by1 > ay2);
}

REFLEX_INLINE bool Reflex::GLX::Contains(Size size, Point point)
{
	return (point.x >= 0.0f) && (point.x < size.w) && (point.y >= 0.0f) && (point.y < size.h);
}

REFLEX_INLINE bool Reflex::GLX::Contains(const Rect & rect, Point point)
{
	auto xy1 = rect.origin + Reinterpret<Point>(rect.size);

	return (point.x >= rect.origin.x) && (point.x < xy1.x) && (point.y >= rect.origin.y) && (point.y < xy1.y);
}

REFLEX_INLINE bool Reflex::GLX::Contains(const Rect & rect, const Rect & inner)
{
	if (inner.origin.x >= rect.origin.x && inner.origin.y >= rect.origin.y)
	{
		auto far0 = rect.origin + Reinterpret<Point>(rect.size);

		auto far1 = inner.origin + Reinterpret<Point>(inner.size);

		return (far1.x <= far0.x && far1.y <= far0.y);
	}

	return false;
}

inline bool Reflex::GLX::CircleContains(Float diameter, Point point)
{
	Float radius = (diameter * 0.5f);

	Float x = point.x - radius;

	Float y = point.y - radius;

	Float delta = (x * x) + (y * y);

	return delta <= (radius * radius);
}

REFLEX_INLINE Reflex::GLX::Rect Reflex::GLX::Rectify(const Rect & rect)
{
	Rect rtn = rect;

	if (rect.size.w < 0.0f)
	{
		rtn.origin.x += rtn.size.w;
		rtn.size.w *= -1.0f;
	}

	if (rect.size.h < 0.0f)
	{
		rtn.origin.y += rtn.size.h;
		rtn.size.h *= -1.0f;
	}

	return rtn;
}

REFLEX_INLINE Reflex::GLX::Rect Reflex::GLX::Indent(const Rect & rect, Float amount)
{
	Float amt2 = amount * 2.0f;

	return { { rect.origin.x + amount, rect.origin.y + amount }, { rect.size.w - amt2, rect.size.h - amt2 } };
}

REFLEX_INLINE Reflex::GLX::Rect Reflex::GLX::Indent(const Rect & rect, Size amount)
{
	Size amt2 = amount * 2.0f;

	return { { rect.origin.x + amount.w, rect.origin.y + amount.h }, { rect.size.w - amt2.w, rect.size.h - amt2.h } };
}

REFLEX_INLINE Reflex::GLX::Rect Reflex::GLX::Indent(Size size, const Margin & amount)
{
	Rect rect = { Reinterpret<Point>(amount.near), size };

	rect.size.w -= amount.near.w + amount.far.w;

	rect.size.h -= amount.near.h + amount.far.h;

	return rect;
}

REFLEX_INLINE Reflex::GLX::Rect Reflex::GLX::Indent(const Rect & rect, const Margin & amount)
{
	return { rect.origin + Reinterpret<Point>(amount.near), rect.size - Sum(amount) };
}

REFLEX_INLINE Reflex::GLX::Rect Reflex::GLX::Expand(const Rect & rect, Float amount)
{
	Float amt2 = amount * 2.0f;

	return { { rect.origin.x - amount, rect.origin.y - amount }, { rect.size.w + amt2, rect.size.h + amt2 } };
}

REFLEX_INLINE Reflex::GLX::Rect Reflex::GLX::Expand(const Rect & rect, Size amount)
{
	Size amt2 = amount * 2.0f;

	return { { rect.origin.x - amount.w, rect.origin.y - amount.h }, { rect.size.w + amt2.w, rect.size.h + amt2.h } };
}

REFLEX_INLINE Reflex::GLX::Size Reflex::GLX::Sum(const Margin & margin)
{
	return margin.near + margin.far;
}

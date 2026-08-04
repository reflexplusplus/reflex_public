#pragma once

#include "types.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE>
	struct Point
	{
		TYPE x = 0;
		TYPE y = 0;
	};

	template <class TYPE>
	struct Size
	{
		TYPE w = 0;
		TYPE h = 0;
	};

	template <class TYPE>
	struct Rect
	{
		Point <TYPE> origin;

		Size <TYPE> size;
	};


	template <class TYPE> constexpr Point <TYPE> MakePoint(TYPE x, TYPE y);


	template <class TYPE> Point <TYPE> operator-(const Point <TYPE> & a);


	template <class TYPE> Point <TYPE> operator+(const Point <TYPE> & a, const Point <TYPE> & b);

	template <class TYPE> Point <TYPE> operator-(const Point <TYPE> & a, const Point <TYPE> & b);

	template <class TYPE> Point <TYPE> operator*(const Point <TYPE> & point, const Size <TYPE> & size);

	template <class TYPE> Point <TYPE> operator/(const Point <TYPE> & point, const Size <TYPE> & size);


	template <class TYPE> Point <TYPE> operator+(const Point <TYPE> & point, Float value);

	template <class TYPE> Point <TYPE> operator-(const Point <TYPE> & point, Float value);


	template <class TYPE> Point <TYPE> & operator+=(Point <TYPE> & self, const Point <TYPE> & value);

	template <class TYPE> Point <TYPE> & operator-=(Point <TYPE> & self, const Point <TYPE> & value);

	template <class TYPE> Point <TYPE> & operator*=(Point <TYPE> & self, const Size <TYPE> & size);

	template <class TYPE> Point <TYPE> & operator/=(Point <TYPE> & self, const Size <TYPE> & size);


	template <class TYPE> Point <TYPE> & operator+=(Point <TYPE> & self, Float value);

	template <class TYPE> Point <TYPE> & operator-=(Point <TYPE> & self, Float value);


	template <class TYPE> bool operator==(const Point <TYPE> & a, const Point <TYPE> & b);

	template <class TYPE> bool operator!=(const Point <TYPE> & a, const Point <TYPE> & b);


	template <class TYPE> Point <TYPE> Min(const Point <TYPE> & a, const Point <TYPE> & b);

	template <class TYPE> Point <TYPE> Max(const Point <TYPE> & a, const Point <TYPE> & b);


	template <class TYPE> constexpr Size <TYPE> MakeSize(TYPE x, TYPE y);

	template <class TYPE> constexpr Size <TYPE> MakeSize(TYPE value);


	template <class TYPE> Size <TYPE> operator-(const Size <TYPE> & size);


	template <class TYPE> Size <TYPE> operator+(const Size <TYPE> & a, const Size <TYPE> & b);

	template <class TYPE> Size <TYPE> operator-(const Size <TYPE> & a, const Size <TYPE> & b);

	template <class TYPE> Size <TYPE> operator*(const Size <TYPE> & a, const Size <TYPE> & b);

	template <class TYPE> Size <TYPE> operator/(const Size <TYPE> & a, const Size <TYPE> & b);


	template <class TYPE> Size <TYPE> operator+(const Size <TYPE> & a, Float scale);

	template <class TYPE> Size <TYPE> operator-(const Size <TYPE> & a, Float scale);

	template <class TYPE> Size <TYPE> operator*(const Size <TYPE> & a, Float scale);

	template <class TYPE> Size <TYPE> operator/(const Size <TYPE> & a, Float scale);


	template <class TYPE> Size <TYPE> & operator+=(Size <TYPE> & self, const Size <TYPE> & size);

	template <class TYPE> Size <TYPE> & operator-=(Size <TYPE> & self, const Size <TYPE> & size);

	template <class TYPE> Size <TYPE> & operator*=(Size <TYPE> & self, const Size <TYPE> & scale);

	template <class TYPE> Size <TYPE> & operator/=(Size <TYPE> & self, const Size <TYPE> & scale);


	template <class TYPE> Size <TYPE> & operator+=(Size <TYPE> & self, Float scale);

	template <class TYPE> Size <TYPE> & operator-=(Size <TYPE> & self, Float scale);

	template <class TYPE> Size <TYPE> & operator*=(Size <TYPE> & self, Float scale);

	template <class TYPE> Size <TYPE> & operator/=(Size <TYPE> & self, Float scale);


	template <class TYPE> bool operator==(const Size <TYPE> & a, const Size <TYPE> & value);

	template <class TYPE> bool operator!=(const Size <TYPE> & a, const Size <TYPE> & value);


	template <class TYPE> Size <TYPE> Min(const Size <TYPE> & a, const Size <TYPE> & b);

	template <class TYPE> Size <TYPE> Max(const Size <TYPE> & a, const Size <TYPE> & b);


	template <class TYPE> bool operator==(const Rect <TYPE> & a, const Rect <TYPE> & b);

	template <class TYPE> bool operator!=(const Rect <TYPE> & a, const Rect <TYPE> & b);

}




//
//impl

template <class TYPE> constexpr REFLEX_INLINE Reflex::Point <TYPE> Reflex::MakePoint(TYPE x, TYPE y)
{
	return { x, y };
}

template <class TYPE> constexpr REFLEX_INLINE Reflex::Size <TYPE> Reflex::MakeSize(TYPE w, TYPE h)
{
	return { w, h };
}

template <class TYPE> constexpr REFLEX_INLINE Reflex::Size <TYPE> Reflex::MakeSize(TYPE size)
{
	return { size, size };
}

template <class TYPE> REFLEX_INLINE Reflex::Point <TYPE> Reflex::operator+(const Point <TYPE> & self, Float value)
{
	return { self.x + value, self.y + value };
}

template <class TYPE> REFLEX_INLINE Reflex::Point <TYPE> Reflex::operator-(const Point <TYPE> & self, Float value)
{
	return { self.x - value, self.y - value };
}

template <class TYPE> REFLEX_INLINE Reflex::Point <TYPE> Reflex::operator*(const Point <TYPE> & self, const Size <TYPE> & scale)
{
	return { self.x * scale.w, self.y * scale.h };
}

template <class TYPE> REFLEX_INLINE Reflex::Point <TYPE> Reflex::operator/(const Point <TYPE> & self, const Size <TYPE> & scale)
{
	return { self.x / scale.w, self.y / scale.h };
}

template <class TYPE> REFLEX_INLINE Reflex::Point <TYPE> & Reflex::operator+=(Point <TYPE> & self, const Point <TYPE> & point)
{
	self.x += point.x;
	self.y += point.y;

	return self;
}

template <class TYPE> REFLEX_INLINE Reflex::Point <TYPE> & Reflex::operator+=(Point <TYPE> & self, Float value)
{
	self.x += value;
	self.y += value;

	return self;
}

template <class TYPE> REFLEX_INLINE Reflex::Point <TYPE> & Reflex::operator-=(Point <TYPE> & self, const Point <TYPE> & point)
{
	self.x -= point.x;
	self.y -= point.y;

	return self;
}

template <class TYPE> REFLEX_INLINE Reflex::Point <TYPE> & Reflex::operator-=(Point <TYPE> & self, Float value)
{
	self.x += value;
	self.y += value;

	return self;
}

template <class TYPE> REFLEX_INLINE Reflex::Point <TYPE> & Reflex::operator*=(Point <TYPE> & self, const Size <TYPE> & scale)
{
	self.x *= scale.w;
	self.y *= scale.h;

	return self;
}

template <class TYPE> REFLEX_INLINE Reflex::Point <TYPE> & Reflex::operator/=(Point <TYPE> & self, const Size <TYPE> & scale)
{
	self.x /= scale.w;
	self.y /= scale.h;

	return self;
}

template <class TYPE> REFLEX_INLINE bool Reflex::operator==(const Point <TYPE> & a, const Point <TYPE> & b)
{
	return Reinterpret<UInt64>(a) == Reinterpret<UInt64>(b);
}

template <class TYPE> REFLEX_INLINE bool Reflex::operator!=(const Point <TYPE> & a, const Point <TYPE> & b)
{
	return Reinterpret<UInt64>(a) != Reinterpret<UInt64>(b);
}

template <class TYPE> REFLEX_INLINE Reflex::Point <TYPE> Reflex::operator+(const Point <TYPE> & a, const Point <TYPE> & b)
{
	return { a.x + b.x, a.y + b.y };
}

template <class TYPE> REFLEX_INLINE Reflex::Point <TYPE> Reflex::operator-(const Point <TYPE> & a, const Point <TYPE> & b)
{
	return { a.x - b.x, a.y - b.y };
}

template <class TYPE> REFLEX_INLINE Reflex::Point <TYPE> Reflex::operator-(const Point <TYPE> & point)
{
	return { -point.x, -point.y };
}

template <class TYPE> REFLEX_INLINE Reflex::Point <TYPE> Reflex::Min(const Point <TYPE> & a, const Point <TYPE> & b)
{
	return { Reflex::Min(a.x, b.x), Reflex::Min(a.y, b.y) };
}

template <class TYPE> REFLEX_INLINE Reflex::Point <TYPE> Reflex::Max(const Point <TYPE> & a, const Point <TYPE> & b)
{
	return { Reflex::Max(a.x, b.x), Reflex::Max(a.y, b.y) };
}

template <class TYPE> REFLEX_INLINE Reflex::Size <TYPE> Reflex::operator+(const Size <TYPE> & a, const Size <TYPE> & b)
{
	return { a.w + b.w, a.h + b.h };
}

template <class TYPE> REFLEX_INLINE Reflex::Size <TYPE> Reflex::operator+(const Size <TYPE> & a, Float value)
{
	return { a.w + value, a.h + value };
}

template <class TYPE> REFLEX_INLINE Reflex::Size <TYPE> Reflex::operator-(const Size <TYPE> & a, const Size <TYPE> & b)
{
	return { a.w - b.w, a.h - b.h };
}

template <class TYPE> REFLEX_INLINE Reflex::Size <TYPE> Reflex::operator-(const Size <TYPE> & a, Float value)
{
	return { a.w - value, a.h - value };
}

template <class TYPE> REFLEX_INLINE Reflex::Size <TYPE> Reflex::operator*(const Size <TYPE> & a, const Size <TYPE> & scale)
{
	return { a.w * scale.w, a.h * scale.h };
}

template <class TYPE> REFLEX_INLINE Reflex::Size <TYPE> Reflex::operator*(const Size <TYPE> & a, Float value)
{
	return { a.w * value, a.h * value };
}

template <class TYPE> REFLEX_INLINE Reflex::Size <TYPE> Reflex::operator/(const Size <TYPE> & a, const Size <TYPE> & scale)
{
	return { a.w / scale.w, a.h / scale.h };
}

template <class TYPE> REFLEX_INLINE Reflex::Size <TYPE> Reflex::operator/(const Size <TYPE> & a, Float value)
{
	auto rcp = 1.0f / value;

	return { a.w * rcp, a.h * rcp };
}

template <class TYPE> REFLEX_INLINE Reflex::Size <TYPE> & Reflex::operator+=(Size <TYPE> & self, const Size <TYPE> & size)
{
	self.w += size.w;
	self.h += size.h;

	return self;
}

template <class TYPE> REFLEX_INLINE Reflex::Size <TYPE> & Reflex::operator+=(Size <TYPE> & self, Float value)
{
	self.w += value;
	self.h += value;

	return self;
}

template <class TYPE> REFLEX_INLINE Reflex::Size <TYPE> & Reflex::operator-=(Size <TYPE> & self, Float value)
{
	self.w -= value;
	self.h -= value;

	return self;
}

template <class TYPE> REFLEX_INLINE Reflex::Size <TYPE> & Reflex::operator-=(Size <TYPE> & self, const Size <TYPE> & size)
{
	self.w -= size.w;
	self.h -= size.h;

	return self;
}

template <class TYPE> REFLEX_INLINE Reflex::Size <TYPE> & Reflex::operator*=(Size <TYPE> & self, const Size <TYPE> & scale)
{
	self.w *= scale.w;
	self.h *= scale.h;

	return self;
}

template <class TYPE> REFLEX_INLINE Reflex::Size <TYPE> & Reflex::operator*=(Size <TYPE> & self, Float value)
{
	self.w *= value;
	self.h *= value;

	return self;
}

template <class TYPE> REFLEX_INLINE Reflex::Size <TYPE> & Reflex::operator/=(Size <TYPE> & self, const Size <TYPE> & scale)
{
	self.w /= scale.w;
	self.h /= scale.h;

	return self;
}

template <class TYPE> REFLEX_INLINE bool Reflex::operator==(const Size <TYPE> & a, const Size <TYPE> & b)
{
	return Reinterpret<UInt64>(a) == Reinterpret<UInt64>(b);
}

template <class TYPE> REFLEX_INLINE bool Reflex::operator!=(const Size <TYPE> & a, const Size <TYPE> & b)
{
	return Reinterpret<UInt64>(a) != Reinterpret<UInt64>(b);
}

template <class TYPE> REFLEX_INLINE Reflex::Size <TYPE> Reflex::operator-(const Size <TYPE> & size)
{
	return { -size.w, -size.h };
}

template <class TYPE> REFLEX_INLINE Reflex::Size <TYPE> Reflex::Min(const Size <TYPE> & a, const Size <TYPE> & b)
{
	return { Min(a.w, b.w), Min(a.h, b.h) };
}

template <class TYPE> REFLEX_INLINE Reflex::Size <TYPE> Reflex::Max(const Size <TYPE> & a, const Size <TYPE> & b)
{
	return { Max(a.w, b.w), Max(a.h, b.h) };
}

template <class TYPE> REFLEX_INLINE bool Reflex::operator==(const Rect <TYPE> & a, const Rect <TYPE> & b)
{
	return (a.origin == b.origin) && (b.size == b.size);
}

template <class TYPE> REFLEX_INLINE bool Reflex::operator!=(const Rect <TYPE> & a, const Rect <TYPE> & b)
{
	return (a.origin != b.origin) || (a.size != b.size);
}

REFLEX_ASSERT_RAW(Reflex::Point<Reflex::Float32>)
REFLEX_ASSERT_RAW(Reflex::Size<Reflex::Float32>)
REFLEX_ASSERT_RAW(Reflex::Rect<Reflex::Float32>)



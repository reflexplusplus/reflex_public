#pragma once

#include "../[require].h"
#include "../../functions/colour.h"




//
//Detail

namespace Reflex::GLX::Detail
{

	constexpr Float kCornerRadians = k2Pif * 0.25f;

	constexpr UInt kNumCornerStep = 7;

	constexpr Float kCornerStepRadians = kCornerRadians / Float(kNumCornerStep);


	template <Reflex::AllocatePolicy POLICY> void AddQuad(Array <Point> & buffer, Float x1, Float y1, Float x2, Float y2);

	template <Reflex::AllocatePolicy POLICY> void AddQuad(Array <System::ColourPoint> & buffer, const Colour & colour, Float x1, Float y1, Float x2, Float y2);


	inline Float Magnitude(Point point)
	{
		return SquareRoot(Square(point.x) + Square(point.y));
	}

	inline Float DotProduct(Point a, Point b)
	{
		return (a.x * b.x) + (a.y * b.y);
	}

	inline Point Normalise(Point p)
	{
		auto len2 = Square(p.x) + Square(p.y);

		if (len2 < Core::kRoundingTolerance) return { 0.0f, 0.0f }; // or just return p if you want to keep directionless vectors unchanged

		Float rcp = 1.0f / SquareRoot(len2);

		return { p.x * rcp, p.y * rcp };
	}


	REFLEX_INLINE Float TopLeft(const Margin & margin) { return margin.near.w; }

	REFLEX_INLINE Float TopRight(const Margin & margin) { return margin.near.h; }

	REFLEX_INLINE Float BottomLeft(const Margin & margin) { return margin.far.w; }

	REFLEX_INLINE Float BottomRight(const Margin & margin) { return margin.far.h; }


	REFLEX_INLINE Size TopLeft(const Margin & margin, Size min) { return Min(Reflex::MakeSize(TopLeft(margin)), min); }

	REFLEX_INLINE Size TopRight(const Margin & margin, Size min) { return Min(Reflex::MakeSize(TopRight(margin)), min); }

	REFLEX_INLINE Size BottomLeft(const Margin & margin, Size min) { return Min(Reflex::MakeSize(BottomLeft(margin)), min); }

	REFLEX_INLINE Size BottomRight(const Margin & margin, Size min) { return Min(Reflex::MakeSize(BottomRight(margin)), min); }


	template <class BUFFER> REFLEX_INLINE void AllocateExtra(BUFFER & data, UInt size)
	{
		data.Allocate(data.GetSize() + size);
	}

}

namespace Reflex::GLX
{

	REFLEX_INLINE System::ColourPoint operator+(const System::ColourPoint & a, const System::fPoint & b)
	{
		auto r = a;

		RemoveConst(Reinterpret<Point>(r.a)) += Cast<Point>(b);

		return r;
	}

	REFLEX_INLINE System::ColourPoint operator-(const System::ColourPoint & a, const System::fPoint & b)
	{
		auto r = a;

		RemoveConst(Reinterpret<Point>(r.a)) -= Cast<Point>(b);

		return r;
	}

	REFLEX_INLINE System::ColourPoint operator*(const System::ColourPoint & a, const Scale & b)
	{
		auto r = a;

		RemoveConst(Reinterpret<Point>(r.a)) *= b;

		return r;
	}

	REFLEX_INLINE Point operator+(const System::fPoint & a, const System::fSize & b)
	{
		return Cast<Point>(a) + Reinterpret<Point>(b);
	}

}




//
//impl

template <Reflex::AllocatePolicy POLICY> inline void Reflex::GLX::Detail::AddQuad(Array <Point> & buffer, Float x1, Float y1, Float x2, Float y2)
{
	auto pts = Extend<POLICY>(buffer, 6);

	Point a = { x1, y1 };
	Point b = { x1, y2 };
	Point c = { x2, y1 };
	Point d = { x2, y2 };

	pts[0] = a; 
	pts[1] = b; 
	pts[2] = c;
	pts[3] = c; 
	pts[4] = b; 
	pts[5] = d;
}

template <Reflex::AllocatePolicy POLICY> inline void Reflex::GLX::Detail::AddQuad(Array <System::ColourPoint> & buffer, const Colour & colour, Float x1, Float y1, Float x2, Float y2)
{
	auto pts = Extend<POLICY>(buffer, 6);

	System::ColourPoint a = { x1, y1, colour };
	System::ColourPoint b = { x1, y2, colour };
	System::ColourPoint c = { x2, y1, colour };
	System::ColourPoint d = { x2, y2, colour };

	pts[0] = a;
	pts[1] = b;
	pts[2] = c;
	pts[3] = c;
	pts[4] = b;
	pts[5] = d;
}


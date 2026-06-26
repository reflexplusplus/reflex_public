#pragma once

#include "reflex/glx/detail/functions/geometry.h"
#include "../library.h"




//
//declarations

REFLEX_NS(Reflex::GLX::Detail)

REFLEX_INLINE Pair <Float> SinCos(Float a) { return { Sin(a), Cos(a) }; }

template <bool FLIP = false> REFLEX_INLINE void AddGradientImpl(ColourPoints & points, Float x1, Float y1, Float x2, Float y2, const Colour & tl_colour, const Colour & tr_colour, const Colour & bl_colour, const Colour & br_colour)
{
	auto region = Extend(points, 6);

	System::ColourPoint tl = { Reflex::MakePoint(x1, y1), tl_colour };
	System::ColourPoint tr = { Reflex::MakePoint(x2, y1), tr_colour };
	System::ColourPoint bl = { Reflex::MakePoint(x1, y2), bl_colour };
	System::ColourPoint br = { Reflex::MakePoint(x2, y2), br_colour };

	if constexpr (FLIP)
	{
		region[0] = tl;
		region[1] = br;
		region[2] = tr;

		region[3] = tl;
		region[4] = bl;
		region[5] = br;
	}
	else
	{
		region[0] = tl;
		region[1] = bl;
		region[2] = tr;

		region[3] = tr;
		region[4] = br;
		region[5] = bl;
	}
}

REFLEX_INLINE void AddGradientImpl(ColourPoints & points, Float x1, Float y1, Float x2, Float y2, bool y, const Colour & from, const Colour & to)
{
	Colour colours[4];

	colours[0] = from;
	colours[1] = y ? from : to;
	colours[2] = y ? to : from;
	colours[3] = to;

	AddGradientImpl
	(
		points,
		x1,
		y1,
		x2,
		y2,
		colours[0],
		colours[1],
		colours[2],
		colours[3]
	);
}

REFLEX_INLINE Float CalculateCircleStep(Size size)
{
	constexpr Float kReferenceNumStep = Float(kNumCornerStep * 4);

	constexpr Float kRcpReferenceSize = 1.0f / 64.0f;

	Float max = Max(size.w, size.h) * g_library->m_fdpifactor;

	Float nedge = Max(max * kRcpReferenceSize, 1.0f) * kReferenceNumStep;

	return k2Pif / nedge;
}

REFLEX_INLINE Point CalculateCirclePoint(Size radius, Float angle_radians)
{
	return { Sin(angle_radians) * radius.w, Cos(angle_radians) * radius.h };
}

REFLEX_INLINE void AddRoundCap(Points& out, Point center, Point dir, Size half_width_xy, UInt segments)
{
	Float base = Float(std::atan2(dir.y, dir.x)) - kPif * 0.5f;

	// start radial from base angle (one sin/cos pair)
	auto [sn,cs] = SinCos(base);

	// constant increment = PI / segments
	auto inc = kPif / Float(segments);
	auto [s,c] = SinCos(inc);

	Point prev = center + Point{ cs, sn } *half_width_xy;

	auto ptr = Extend(out, segments * 3u).data;

	REFLEX_LOOP(i, segments)
	{
		// rotate (cs,sn) by inc
		auto cs1 = cs * c - sn * s;
		auto sn1 = sn * c + cs * s;
		
		cs = cs1;
		sn = sn1;

		Point next = center + Point{ cs, sn } * half_width_xy;

		*ptr++ = center;
		*ptr++ = prev;
		*ptr++ = next;

		prev = next;
	}
}

//struct SinCosIncrementer
//{
//	SinCosIncrementer(Point start, Float step)
//		: v(start)
//		, sin(Sin(step))
//		, cos(Cos(step))
//	{
//	}
//
//	Point Current() const { return v; }
//
//	void Step() { v = { v.x * cos - v.y * sin, v.x * sin + v.y * cos }; }
//
//	Point v;   // current vector (radius applied)
//	Float sin;   // sin(step)
//	Float cos;   // cos(step)
//};

constexpr Float kTopLeftCornerOrigin = k2Pif * 0.5f;

constexpr Float kTopRightCornerOrigin = k2Pif * 0.75f;

constexpr Float kBottomRightCornerOrigin = 0.0f;

constexpr Float kBottomLeftCornerOrigin = Detail::kCornerRadians;

REFLEX_END

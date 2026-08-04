#include "drawing.h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

constexpr Tuple <Point,Point,Point> MakeTriangle(Float x1, Float y1, Float x2, Float y2, Float x3, Float y3)
{
	return { {x1, y1}, {x2, y2}, {x3, y3} };
}

const Tuple <Point, Point, Point> kTriangles[kNumAlignment] =
{
	MakeTriangle(0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f),
	MakeTriangle(0.0f, 1.0f, 0.5f, 0.0f, 1.0f, 1.0f),
	MakeTriangle(0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f),

	MakeTriangle(1.0f, 1.0f, 0.0f, 0.5f, 1.0f, 0.0f),
	MakeTriangle(0.0f, 0.0f, 0.5f, 1.0f, 1.0f, 0.0f),
	MakeTriangle(0.0f, 0.0f, 1.0f, 0.5f, 0.0f, 1.0f),

	MakeTriangle(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f),
	MakeTriangle(0.0f, 0.0f, 0.5f, 1.0f, 1.0f, 0.0f),
	MakeTriangle(1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f),
};

Pair <Point> ShortenLine(Point a, Point b, Float n, Size pixel_size)
{
	auto dir_x = b.x - a.x;
	auto dir_y = b.y - a.y;

	auto t0 = Detail::Magnitude({ dir_x, dir_y });

	auto mult = n / t0;

	auto offset_x = dir_x * pixel_size.w * mult;
	auto offset_y = dir_y * pixel_size.h * mult;

	return { {a.x + offset_x, a.y + offset_y}, {b.x - offset_x, b.y - offset_y} };
}

REFLEX_INLINE void AppendCornerArc(Points &out, Point start, Point ctrl, Point end, UInt nstep)
{
	auto mult = 1.0f / ToFloat32(nstep);

	for (UInt i = 0; i <= nstep; ++i)
	{
		Float t = ToFloat32(i) * mult;
		Float omt = 1.0f - t;
		Float omt2 = omt * omt;
		Float t2 = t * t;
		Float two = 2.0f * omt * t;

		Point p = { omt2 * start.x + two * ctrl.x + t2 * end.x, omt2 * start.y + two * ctrl.y + t2 * end.y };

		if (i == 0 && out.GetSize())
		{
			auto prev = out[out.GetSize() - 1];

			if (prev.x == p.x && prev.y == p.y) continue;
		}

		out.Push<kAllocateNone>(p);
	}
}

REFLEX_NOINLINE void AddRoundedTriangle(Points & poly, const Rect & rect, Float corner, Alignment direction, Size pixel_size)
{
	const auto & shape = kTriangles[direction];

	auto o = rect.origin;
	auto s = rect.size;

	auto A = (shape.a * s) + o;
	auto B = (shape.b * s) + o;
	auto C = (shape.c * s) + o;

	auto AB = ShortenLine(A, B, corner, pixel_size); // AB.a near A, AB.b near B
	auto BC = ShortenLine(B, C, corner, pixel_size); // BC.a near B, BC.b near C
	auto CA = ShortenLine(C, A, corner, pixel_size); // CA.a near C, CA.b near A

	UInt nstep = Truncate(Max(corner, 3.0f));

	poly.Allocate(poly.GetSize() + (3 * nstep + 4));

	AppendCornerArc(poly, CA.b, A, AB.a, nstep);

	poly.Push<kAllocateNone>(AB.b);

	AppendCornerArc(poly, AB.b, B, BC.a, nstep);

	poly.Push<kAllocateNone>(BC.b);

	AppendCornerArc(poly, BC.b, C, CA.a, nstep);

	poly.Push<kAllocateNone>(CA.b);
}

REFLEX_INLINE bool SamePt(Point a, Point b)
{
	constexpr Float kEps = 1e-5f;

	return Abs(a.x - b.x) <= kEps && Abs(a.y - b.y) <= kEps;
}

REFLEX_INLINE void TriFan(Points & output, const Points & poly, Point c)
{
	UInt n = poly.GetSize();

	if (n < 3) return;

	output.Allocate(output.GetSize() + (n * 3));

	REFLEX_LOOP(idx, n - 1)
	{
		auto tri = Extend<kAllocateNone>(output, 3);
		
		tri[0] = c;
		tri[1] = poly[idx];
		tri[2] = poly[idx + 1];
	}

	if (!SamePt(poly[n - 1], poly[0]))
	{
		auto tri = Extend<kAllocateNone>(output, 3);

		tri[0] = c;
		tri[1] = poly[n - 1];
		tri[2] = poly[0];
	}
}

REFLEX_END_INTERNAL

void Reflex::GLX::AddTriangleOutline(Points & points, const Rect & container, Float width, Alignment direction, Size pixel_size)
{
	auto rect = Indent(container, MakeSize(width * 0.5f) * pixel_size);

	auto & shape = kTriangles[direction];

	auto & origin = rect.origin;

	auto size = rect.size;

	auto a = (shape.a * size) + origin;
	auto b = (shape.b * size) + origin;
	auto c = (shape.c * size) + origin;

	AddPath(points, { a, b, c }, true, width, pixel_size);
}

void Reflex::GLX::AddTriangleFill(Points & points, const Rect & rect, Alignment direction)
{
	auto & shape = kTriangles[direction];

	auto & origin = rect.origin;

	auto size = rect.size;

	auto ppoints = Extend(points, 3).data;

	*ppoints++ = (shape.a * size) + origin;
	*ppoints++ = (shape.b * size) + origin;
	*ppoints++ = (shape.c * size) + origin;
}

void Reflex::GLX::AddRoundedTriangleOutline(Points & points, const Rect & container, Float width, Float corner, Alignment direction, Size pixel_size)
{
	corner = Max(corner, width);

	auto rect = Indent(container, MakeSize(width * 0.5f) * pixel_size);

	GLX_GET_POINT_WORKSPACE(poly);

	AddRoundedTriangle(poly, rect, corner, direction, pixel_size);
	
	AddPath(points, poly, true, width, pixel_size);
}

void Reflex::GLX::AddRoundedTriangleFill(Points & points, const Rect & rect, Float corner, Alignment direction, Size pixel_size)
{
	GLX_GET_POINT_WORKSPACE(poly);

	auto o = rect.origin;
	auto s = rect.size;

	AddRoundedTriangle(poly, rect, corner, direction, pixel_size);

	TriFan(points, poly, { o.x + s.w * 0.5f, o.y + s.h * 0.5f });
}

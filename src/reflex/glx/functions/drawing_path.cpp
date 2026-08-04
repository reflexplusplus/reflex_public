#include "drawing.h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

template <bool MITRE1, bool MITRE2> void AddPathSegment(Points & points, Float width, Point p0, Point p1, Point p2, Point p3)
{
	Point line = Normalise(p2 - p1);

	Point normal = Normalise(Reflex::MakePoint(-line.y, line.x));

	Point tangent1 = MITRE1 ? Normalise(Normalise(p1 - p0) + line) : line;

	Point tangent2 = MITRE2 ? Normalise(Normalise(p3 - p2) + line) : line;

	Point miter1 = { -tangent1.y, tangent1.x };

	Point miter2 = { -tangent2.y, tangent2.x };

	miter1 *= Reflex::MakeSize(width / DotProduct(normal, miter1));

	miter2 *= Reflex::MakeSize(width / DotProduct(normal, miter2));

	auto t0 = p1 - miter1;
	auto t1 = p1 + miter1;
	auto t2 = p2 + miter2;
	auto t3 = p2 - miter2;

	auto ppoints = Extend(points, 6).data;

	*ppoints++ = t0;
	*ppoints++ = t1;
	*ppoints++ = t2;
	*ppoints++ = t2;
	*ppoints++ = t3;
	*ppoints++ = t0;
}

REFLEX_INLINE void AddPathExTri(Point * pts, Point a, Point b, Point c)
{
	pts[0] = a;
	pts[1] = b;
	pts[2] = c;
}

REFLEX_INLINE void AddPathExQuad(Point * pts, Point a, Point b, Point c, Point d)
{
	pts[0] = a; pts[1] = b; pts[2] = c;
	pts[3] = c; pts[4] = b; pts[5] = d;
}

struct EdgeInfo { Point dir; Point normal; };

struct AddPathExJoinContext
{
	Points::View path;
	const EdgeInfo* edges;
	Size half_width_xy;
	Float half_width;
	Float miter_limit;
	UInt num_round_segments;
};

REFLEX_INLINE void AddPathExRoundJoin(Points & output, Point center, Point from, Point to, Float half_width, UInt segments)
{
	Float a0 = Float(std::atan2(from.y - center.y, from.x - center.x));
	Float a1 = Float(std::atan2(to.y - center.y, to.x - center.x));

	Float diff = a1 - a0;
	if (diff > kPif) diff -= k2Pif;
	if (diff < -kPif) diff += k2Pif;

	auto ptr = Extend(output, segments * 3).data;

	Float angle_step = diff / Float(segments);
	Float angle = a0 + angle_step;

	Point prev = from;

	REFLEX_LOOP(i, segments)
	{
		Point next = { center.x + half_width * Float(std::cos(angle)), center.y + half_width * Float(std::sin(angle)) };

		*ptr++ = center;
		*ptr++ = prev;
		*ptr++ = next;

		prev = next;
		angle += angle_step;
	}
}

template <PathJoin JOIN> void AddPathExJoin(Points & output, const AddPathExJoinContext & ctx, UInt vertex_idx, UInt prev_edge, UInt next_edge)
{
	constexpr Float eps = 0.0001f;

	auto edges = ctx.edges;
	auto half_width_xy = ctx.half_width_xy;

	Point p = ctx.path[vertex_idx];
	Point n0 = edges[prev_edge].normal;
	Point n1 = edges[next_edge].normal;
	Point wn0 = n0 * half_width_xy;
	Point wn1 = n1 * half_width_xy;
	Float cross = edges[prev_edge].dir.x * edges[next_edge].dir.y - edges[prev_edge].dir.y * edges[next_edge].dir.x;

	const bool left_turn = (cross > eps);
	const bool right_turn = (cross < -eps);

	if constexpr (JOIN == kPathJoinMiter)
	{
		Float dot = Detail::DotProduct(n0, n1);
		Float miter_ratio = 1.0f;
		
		if (dot > -0.9999f)
		{
			miter_ratio = 1.0f / Max(0.0001f, Float(std::sqrt(0.5f * (1.0f + dot))));
		}

		if (left_turn || right_turn)
		{
			// side: +1 => outer is left (p + wn), -1 => outer is right (p - wn)
			const auto side = Reflex::MakeSize(right_turn ? 1.0f : -1.0f);

			const Point outer0 = p + wn0 * side;
			const Point outer1 = p + wn1 * side;

			if ((miter_ratio > ctx.miter_limit))
			{
				AddPathExTri(Extend(output, 3u).data, p, outer0, outer1);
			}
			else
			{
				Point avg = (n0 + n1) * side;

				Float avg_len = Detail::Magnitude(avg);

				if (avg_len > eps)
				{
					avg = avg * Reflex::MakeSize(1.0f / avg_len);

					const Point n0_side = n0 * side;
					Float denom = Abs(Detail::DotProduct(avg, n0_side));

					Float miter_len = ctx.half_width / Max(eps, denom);
					Point miter = p + avg * Reflex::MakeSize(miter_len);

					auto pts = Extend(output, 6u).data;

					AddPathExTri(pts, outer0, miter, p);
					AddPathExTri(pts + 3, miter, outer1, p);
				}
			}

			// inner side always filled
			AddPathExTri(Extend(output, 3u).data, p, p - wn0 * side, p - wn1 * side);
		}
	}
	else if (left_turn || right_turn)
	{
		const Point side0 = left_turn ? -wn0 : wn0;   // outer side offset (right for left turn, left for right turn)
		const Point side1 = left_turn ? -wn1 : wn1;

		const Point from = p + side0;
		const Point to = p + side1;

		if constexpr (JOIN == kPathJoinRound)
		{
			AddPathExRoundJoin(output, p, from, to, ctx.half_width, ctx.num_round_segments);
		}
		else // kPathJoinBevel
		{
			AddPathExTri(Extend(output, 3u).data, p, from, to);
		}

		auto wn_prev = edges[prev_edge].normal * half_width_xy;
		auto wn_next = edges[next_edge].normal * half_width_xy;

		auto a = left_turn ? (p + wn_prev) : (p - wn_prev);
		auto b = left_turn ? (p + wn_next) : (p - wn_next);

		AddPathExTri(Extend(output, 3u).data, p, a, b);
	}
};

REFLEX_INLINE Point AddPathExPerp(Point dir)
{
	return { -dir.y, dir.x };
}

REFLEX_END_INTERNAL

void Reflex::GLX::Detail::AddPathImpl(Points & buffer, const Points::View & points, bool closed, Float width)
{
	width *= 0.5f;

	Int n = points.size;

	AllocateExtra(buffer, n * 6);

	if (closed)
	{
		for (Int p1 = 0; p1 < n; p1++)
		{
			Int p0 = Modulo(p1 - 1, n);

			Int p2 = Modulo(p1 + 1, n);

			Int p3 = Modulo(p1 + 2, n);

			AddPathSegment<true, true>(buffer, width, points[p0], points[p1], points[p2], points[p3]);
		}
	}
	else if (n > 3)
	{
		AddPathSegment<false, true>(buffer, width, kOrigin, points[0], points[1], points[2]);

		REFLEX_LOOP(p1, n - 3) AddPathSegment<true, true>(buffer, width, points[p1], points[p1 + 1], points[p1 + 2], points[p1 + 3]);

		AddPathSegment<true, false>(buffer, width, points[n - 3], points[n - 2], points[n - 1], kOrigin);
	}
	else if (n == 3)
	{
		auto & p0 = points.GetFirst();

		auto & p1 = points[1];

		auto & p2 = points[2];

		AddPathImpl(buffer, { p0, LinearInterpolate(0.5f, p0, p1), p1, p2 }, false, width * 2.0f);
	}
	else if (n == 2)
	{
		auto & p0 = points.GetFirst();

		auto & p3 = points[1];

		auto p1 = LinearInterpolate(0.25, p0, p3);

		auto p2 = LinearInterpolate(0.75, p0, p3);

		AddPathImpl(buffer, { p0, p1, p2, p3 }, false, width * 2.0f);
	}
}

void Reflex::GLX::Detail::AddPathExImpl(Points & output, const Points::View & path, bool closed, Float width, PathJoin join, PathCap cap, Float miter_limit)
{
	constexpr decltype(&AddPathExJoin<kPathJoinMiter>) kJoinFns[kNumLineJoin] = { &AddPathExJoin<kPathJoinMiter>, &AddPathExJoin<kPathJoinRound>, &AddPathExJoin<kPathJoinBevel> };
	constexpr UInt kRoundSegments = 8;	//TODO based on render size and line width

	if (path.size < 2 || width <= 0.0f) return;

	UInt n = path.size;

	Array <EdgeInfo> edges;

	edges.Allocate(n);

	UInt n_1 = n - 1;

	REFLEX_LOOP(i, n_1)
	{
		auto nd = Detail::Normalise(path.data[i + 1] - path.data[i]);
	
		edges.Push<kAllocateNone>({ nd, AddPathExPerp(nd) });
	}

	if (closed)
	{
		auto nd = Detail::Normalise(path.data[0] - path.data[n_1]);
		
		edges.Push<kAllocateNone>({ nd, AddPathExPerp(nd) });
	}

	UInt edge_count = edges.GetSize();

	auto half_width = width * 0.5f;
	auto half_width_xy = Reflex::MakeSize(half_width);

	auto n_main = closed ? n : n_1;
	
	auto tptr = Extend<kAllocateExact>(output, 6u * n_main).data;
	
	REFLEX_LOOP(i, closed ? n : n_1)
	{
		UInt i0 = i;
		UInt i1 = (i + 1) % n;

		Point p0 = path.data[i0];
		Point p1 = path.data[i1];

		Point wn = edges[i].normal * half_width_xy;

		AddPathExQuad(tptr, p0 + wn, p0 - wn, p1 + wn, p1 - wn);

		tptr += 6;
	}

	auto join_fn = kJoinFns[join];

	AddPathExJoinContext ctx = { .path = path, .edges = edges.GetData(), .half_width_xy = half_width_xy, .half_width = half_width, .miter_limit = miter_limit, .num_round_segments = kRoundSegments };

	if (closed)
	{
		REFLEX_LOOP(i, n)
		{
			UInt prev = (i == 0) ? edge_count - 1 : i - 1;
			join_fn(output, ctx, i, prev, i);
		}
	}
	else
	{
		for (UInt i = 1; i < n - 1; ++i) join_fn(output, ctx, i, i - 1, i);

		if (cap == kPathCapRound)
		{
			tptr = Extend(output, kRoundSegments * 3 * 2).data;
			Float rcp = kPif / Float(kRoundSegments);

			{
				Point p = path.data[0];
				Float base = Float(std::atan2(-edges[0].dir.y, -edges[0].dir.x)) - kPif * 0.5f;
				Float angle = base;
				Point radial = { Float(std::cos(angle)), Float(std::sin(angle)) };
				Point prev = p + radial * half_width_xy;

				REFLEX_LOOP(i, kRoundSegments)
				{
					angle = base + rcp * Float(i + 1);
					radial = { Float(std::cos(angle)), Float(std::sin(angle)) };
					Point next = p + radial * half_width_xy;
					*tptr++ = p; 
					*tptr++ = prev; 
					*tptr++ = next;
					prev = next;
				}
			}

			{
				Point p = path.data[n - 1];
				Float base = Float(std::atan2(edges[edge_count - 1].dir.y, edges[edge_count - 1].dir.x)) - kPif * 0.5f;
				Float angle = base;
				Point radial = { Float(std::cos(angle)), Float(std::sin(angle)) };
				Point prev = p + radial * half_width_xy;

				REFLEX_LOOP(i, kRoundSegments)
				{
					angle = base + rcp * Float(i + 1);
					radial = { Float(std::cos(angle)), Float(std::sin(angle)) };
					Point next = p + radial * half_width_xy;
					*tptr++ = p; 
					*tptr++ = prev; 
					*tptr++ = next;
					prev = next;
				}
			}
		}
		else if (cap == kPathCapSquare)
		{
			tptr = Extend(output, 12).data;

			{
				Point p = path.data[0];
				Point d = edges[0].dir;
				Point n0 = edges[0].normal;
				Point wd = d * half_width_xy;
				Point wn = n0 * half_width_xy;
				Point ext = p - wd;
				AddPathExQuad(tptr, ext + wn, ext - wn, p + wn, p - wn);
				tptr += 6;
			}

			{
				Point p = path.data[n - 1];
				Point d = edges[edge_count - 1].dir;
				Point n0 = edges[edge_count - 1].normal;
				Point wd = d * half_width_xy;
				Point wn = n0 * half_width_xy;
				Point ext = p + wd;
				AddPathExQuad(tptr, p + wn, p - wn, ext + wn, ext - wn);
			}
		}
	}
}

Reflex::GLX::Detail::PathRescaler::PathRescaler(Points & out, const Points::View& in, Size pixel_size)
	: output(out)
	, path(in)
	, start(output.GetSize())
	, scale(Reinterpret<Scale>(pixel_size))
{
	Rescale(path, Reciprocal(scale));
}

Reflex::GLX::Detail::PathRescaler::~PathRescaler()
{
	Points::Region region = { output.GetData() + start, output.GetSize() - start };

	Rescale(region, scale);
}

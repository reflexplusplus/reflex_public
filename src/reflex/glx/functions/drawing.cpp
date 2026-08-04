#include "drawing.h"




//
//declarations

REFLEX_NS(Reflex::GLX::Detail)

struct ColourPointsOutput
{
	typedef System::ColourPoint Type;

	ColourPointsOutput(ColourPoints & points, const Colour & colour)
		: points(points),
		colour(colour)
	{
	}

	void Allocate(UInt size)
	{
		points.Allocate(size);
	}

	UInt GetSize() const { return points.GetSize(); }

	auto GetData() { return points.GetData(); }

	template <AllocatePolicy ALLOC = kAllocateOver> void Push(const Point & point)
	{
		points.Push<ALLOC>({ point, colour });
	}

	operator ColourPoints &() { return points; }

	ColourPoints & points;

	const Colour & colour;
};

template <Reflex::AllocatePolicy POLICY> inline void AddQuad(ColourPointsOutput & buffer, Float x1, Float y1, Float x2, Float y2)
{
	GLX::Detail::AddQuad<POLICY>(buffer.points, buffer.colour, x1, y1, x2, y2);
}

REFLEX_END

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

struct RotationTransform
{
public:

	RotationTransform(Float angle, Point origin = kOrigin);

	void operator()(Point & point) const;

	void operator()(System::ColourPoint & point) const;


private:

	Float m_sin, m_cos;

	Point m_origin;
};

REFLEX_INLINE Float IsUniform(const Margin & margin)
{
	auto & near = margin.near;

	auto & far = margin.far;

	return near == far && near.w == near.h ? near.w : 0.0f;
}

REFLEX_INLINE RotationTransform::RotationTransform(Float angle, Point origin)
	: m_sin(Sin(angle)),
	m_cos(Cos(angle)),
	m_origin(origin)
{
}

REFLEX_INLINE void RotationTransform::operator()(Point & point) const
{
	point -= m_origin;

	Point xy =
	{
		(point.x * m_cos) - (point.y * m_sin),
		(point.x * m_sin) + (point.y * m_cos)
	};

	point = xy + m_origin;
}

REFLEX_INLINE void RotationTransform::operator()(System::ColourPoint & colourpoint) const
{
	(*this)(colourpoint.a);
}

template <bool ROUND_CAP = false> REFLEX_INLINE void MakeArc(Points & points, const Rect & rect, Range sweep, Size width, Float step)
{
	REFLEX_ASSERT(sweep.length >= 0.0f);

	const Size radius = rect.size * 0.5f;
	const Size inner = radius - width;
	const Size mid = radius - width * 0.5f;

	const Point center = rect.origin + Reinterpret<Point>(radius);

	const Size half_width_xy = width * Reflex::MakeSize(0.5f);
	const Float a0 = sweep.start;
	const Float a1 = a0 + sweep.length;

	// start sin/cos once
	auto [s0,c0] = SinCos(a0);

	Point p0 = { center.x + radius.w * c0, center.y + radius.h * s0 };
	Point p2 = { center.x + inner.w * c0, center.y + inner.h * s0 };

	Float angle = a0;

	if constexpr (ROUND_CAP)
	{
		Point cap_center = { center.x + mid.w * c0, center.y + mid.h * s0 };
		Point dir = { -mid.w * s0, mid.h * c0 };
		dir = Detail::Normalise(dir);
		AddRoundCap(points, cap_center, -dir, half_width_xy, kNumCornerStep);
	}

	// precompute rotation for step
	auto [ss, cc] = SinCos(step);

	while (angle + step < a1)
	{
		angle += step;

		// rotate (c0,s0) by step
		const Float c1 = c0 * cc - s0 * ss;
		const Float s1 = s0 * cc + c0 * ss;
		c0 = c1;
		s0 = s1;

		Point p1 = { center.x + radius.w * c0, center.y + radius.h * s0 };
		Point p3 = { center.x + inner.w * c0, center.y + inner.h * s0 };

		auto tri = Extend(points, 6u);
		tri[0] = p0; 
		tri[1] = p2; 
		tri[2] = p1;
		tri[3] = p1; 
		tri[4] = p2; 
		tri[5] = p3;

		p0 = p1;
		p2 = p3;
	}

	// final partial step (exact end angle)
	if (angle < a1)
	{
		auto [se,ce] = SinCos(a1);

		Point p1 = { center.x + radius.w * ce, center.y + radius.h * se };
		Point p3 = { center.x + inner.w * ce, center.y + inner.h * se };

		auto tri = Extend(points, 6u);
		tri[0] = p0; 
		tri[1] = p2; 
		tri[2] = p1;
		tri[3] = p1; 
		tri[4] = p2; 
		tri[5] = p3;

		p0 = p1;
		p2 = p3;

		c0 = ce;
		s0 = se;
		angle = a1;
	}

	if constexpr (ROUND_CAP)
	{
		Point cap_center = { center.x + mid.w * c0, center.y + mid.h * s0 };
		Point dir = { -mid.w * s0, mid.h * c0 };
		dir = Detail::Normalise(dir);
		AddRoundCap(points, cap_center, dir, half_width_xy, kNumCornerStep);
	}
}

REFLEX_INLINE Range RectifySweep(Float start, Float length)
{
	if (length < 0.0f)
	{
		return { start + length, -length };
	}
	else
	{
		return { start, length };
	}
}

REFLEX_INLINE void MakePie(Points & points, const Rect & rect, Range sweep, Float step)
{
	REFLEX_ASSERT(sweep.length >= 0.0f);

	const Size radius = rect.size * 0.5f;
	const Point center = rect.origin + Reinterpret<Point>(radius);

	const Float a0 = sweep.start;
	const Float a1 = a0 + sweep.length;

	Float angle = a0;

	// start (one sin/cos pair)
	auto [sn,cs] = SinCos(a0);

	Point p0 = { center.x + radius.w * cs, center.y + radius.h * sn };

	// precompute step rotation
	auto [s,c] = SinCos(step);

	// full steps
	while (angle + step < a1)
	{
		angle += step;

		// rotate (cs,sn) by inc
		auto cs1 = cs * c - sn * s;
		auto sn1 = sn * c + cs * s;
		cs = cs1;
		sn = sn1;

		Point p1 = { center.x + radius.w * cs, center.y + radius.h * sn };

		auto tri = Extend(points, 3u);
		tri[0] = p0;
		tri[1] = center;
		tri[2] = p1;

		p0 = p1;
	}

	// final partial (exact end angle)
	if (angle < a1)
	{
		auto [se,ce] = SinCos(a1);

		Point p1 = { center.x + radius.w * ce, center.y + radius.h * se };

		auto tri = Extend(points, 3u);
		tri[0] = p0;
		tri[1] = center;
		tri[2] = p1;
	}
}

template <class OUTPUT> inline void AddRectOutlineImpl(OUTPUT & output, const Rect & rect, const Margin & width)
{
	Float x1 = rect.origin.x;
	Float y1 = rect.origin.y;
	Float x2 = x1 + rect.size.w;
	Float y2 = y1 + rect.size.h;

	//Float left = width.near.w * pixelsize.w;
	//Float right = width.far.w * pixelsize.w;
	//Float top = width.near.h * pixelsize.h;
	//Float bottom = width.far.h * pixelsize.h;
	Float left = width.near.w;
	Float right = width.far.w;
	Float top = width.near.h;
	Float bottom = width.far.h;

	Detail::AllocateExtra(output, 4 * 6);

	Detail::AddQuad<kAllocateNone>(output, x1, y1, x1 + left, y2);	//left
	Detail::AddQuad<kAllocateNone>(output, x2 - right, y1, x2, y2);	//right
	Detail::AddQuad<kAllocateNone>(output, x1 + left, y1, x2 - right, y1 + top);	//top
	Detail::AddQuad<kAllocateNone>(output, x1 + left, y2 - bottom, x2 - right, y2);	//bottom
}

template <class BUFFER> REFLEX_INLINE void AddRectFillImpl(BUFFER & output, const Rect & rect)
{
	Float x1 = rect.origin.x;
	Float y1 = rect.origin.y;
	Float x2 = x1 + rect.size.w;
	Float y2 = y1 + rect.size.h;

	Detail::AddQuad<kAllocateOver>(output, x1, y1, x2, y2);
}

REFLEX_INLINE void Tesselate(Array <Int32> & cache, const Points::View & input, Points & output)
{
	REFLEX_INLINE_LOCAL(Float32, Area)(const Point * points, UInt32 n)
	{
		Float32 area = 0.0f;

		for (UInt32 p = n - 1, q = 0; q < n; p = q++)
		{
			auto point_q = points[q];
			auto point_p = points[p];

			area += point_p.x * point_q.y - point_q.x * point_p.y;
		}

		return area * 0.5f;
	}
	REFLEX_END

	REFLEX_INLINE_LOCAL(Float32, Inside)(Point a, Point b, Point c, Point point)
	{
		Float32 ax = c.x - b.x;
		Float32 ay = c.y - b.y;

		Float32 bx = a.x - c.x;
		Float32 by = a.y - c.y;

		Float32 cx = b.x - a.x;
		Float32 cy = b.y - a.y;

		Float32 apx = point.x - a.x;
		Float32 apy = point.y - a.y;

		Float32 bpx = point.x - b.x;
		Float32 bpy = point.y - b.y;

		Float32 cpx = point.x - c.x;
		Float32 cpy = point.y - c.y;

		Float32 acrossbp = ax * bpy - ay * bpx;
		Float32 ccrossap = cx * apy - cy * apx;
		Float32 bcrosscp = bx * cpy - by * cpx;

		return ((acrossbp >= 0.0f) && (bcrosscp >= 0.0f) && (ccrossap >= 0.0f));
	}
	REFLEX_END

	REFLEX_INLINE_LOCAL(bool, Snip)(const Int32 * pcache, const Point * points, Int32 u, Int32 v, Int32 w, Int32 n)
	{
		auto apoint = points[pcache[u]];
		auto bpoint = points[pcache[v]];
		auto cpoint = points[pcache[w]];

		if (0.0000000001f > (((bpoint.x - apoint.x) * (cpoint.y - apoint.y)) - ((bpoint.y - apoint.y) * (cpoint.x - apoint.x)))) return false;

		for (Int32 p = 0; p < n; ++p)
		{
			if ((p == u) || (p == v) || (p == w)) continue;

			if (Inside::Call(apoint, bpoint, cpoint, points[pcache[p]])) return false;
		}

		return true;
	}
	REFLEX_END

	UInt32 n = input.size;

	if (n > 3)
	{
		auto points = input.data;

		cache.SetSize(n);

		Int32 * pcache = cache.GetData();

		if (0.0f < Area::Call(points, n))
		{
			REFLEX_LOOP(v, n) pcache[v] = v;
		}
		else
		{
			REFLEX_LOOP(v, n) pcache[v] = (n - 1) - v;
		}

		Int nv = n;

		Int count = 2 * nv;

		for (Int m = 0, v = nv - 1; nv > 2;)
		{
			if (0 >= (count--)) return;	//false;

			Int u = v;

			if (nv <= u) u = 0;

			v = u + 1;

			if (nv <= v) v = 0;

			Int w = v + 1;

			if (nv <= w) w = 0;

			if (Snip::Call(pcache, points, u, v, w, nv))
			{
				Int a = pcache[u];
				Int b = pcache[v];
				Int c = pcache[w];

				auto tri = Extend(output, 3);

				tri[0] = points[a];
				tri[1] = points[b];
				tri[2] = points[c];

				m++;

				Int s, t;

				for (s = v, t = v + 1; t < nv; s++, t++) pcache[s] = pcache[t];

				nv--;

				count = 2 * nv;
			}
		}
	}
	else if (n == 3)
	{
		output.Append(input);
	}
}

REFLEX_INLINE void AddCompoundPolygonFillImpl(Array <Int32> & cache, Points & output, const ArrayView <Points::View> & input, FillRule fill_rule)
{
	struct CompoundContour
	{
		Points points;
		Float32 ymin = 0.0f;
		Float32 ymax = 0.0f;
	};

	struct ScanEdge
	{
		Float x = 0.0f;
		Float x0 = 0.0f;
		Float x1 = 0.0f;
		Int winding = 0;
	};

	REFLEX_INLINE_LOCAL(bool, BuildContour)(CompoundContour & contour, const Points::View & input)
	{
		if (input.size < 3) return false;

		UInt count = input.size;

		while (count > 2 && input[count - 1] == input[0])
		{
			--count;
		}

		if (count < 3) return false;

		contour.points.Append({ input.data, count });
		contour.ymin = contour.points[0].y;
		contour.ymax = contour.points[0].y;

		for (auto & point : contour.points)
		{
			contour.ymin = Min(contour.ymin, point.y);
			contour.ymax = Max(contour.ymax, point.y);
		}

		return true;
	}
	REFLEX_END

	REFLEX_INLINE_LOCAL(Float,InterpolateXAtY)(Point a, Point b, Float y)
	{
		if (a.y == b.y) return a.x;

		Float t = (y - a.y) / (b.y - a.y);
		
		return a.x + ((b.x - a.x) * t);
	}
	REFLEX_END

	REFLEX_INLINE_LOCAL(void, AddScanEdge)(Array <ScanEdge> & active, Point a, Point b, Float y0, Float ym, Float y1)
	{
		auto & edge = active.Push<kAllocateNone>();

		Float inv = 1.0f / (b.y - a.y);
		Float dx = b.x - a.x;

		edge.x = a.x + dx * ((ym - a.y) * inv);
		edge.x0 = a.x + dx * ((y0 - a.y) * inv);
		edge.x1 = a.x + dx * ((y1 - a.y) * inv);
		edge.winding = a.y < b.y ? 1 : -1;
	}
	REFLEX_END

	REFLEX_INLINE_LOCAL(void, Emit)(Points & output, Float y0, Float y1, const ScanEdge & left, const ScanEdge & right)
	{
		Point p0 = { left.x0, y0 };
		Point p1 = { left.x1, y1 };
		Point p2 = { right.x0, y0 };
		Point p3 = { right.x1, y1 };

		auto tri = Extend<kAllocateNone>(output, 6u);
		tri[0] = p0;
		tri[1] = p1;
		tri[2] = p2;
		tri[3] = p2;
		tri[4] = p1;
		tri[5] = p3;
	}
	REFLEX_END

	Array <CompoundContour> contours;
	
	contours.Allocate(input.size);

	for (auto i : input)
	{
		CompoundContour contour;
		
		if (BuildContour::Call(contour, i))
		{
			contours.Push(std::move(contour));
		}
	}

	if (!contours) return;

	UInt total_points = 0;
	for (auto & contour : contours)
	{
		total_points += contour.points.GetSize();
	}

	Array <Float> ys;
	ys.Allocate(total_points);

	for (auto & contour : contours)
	{
		for (auto & point : contour.points)
		{
			ys.Push(point.y);
		}
	}

	Sort(ys);

	UInt unique_count = 0;
	
	REFLEX_LOOP(i, ys.GetSize())
	{
		if (!i || ys[i] != ys[i - 1])
		{
			ys[unique_count++] = ys[i];
		}
	}

	ys.SetSize(unique_count);

	Array <ScanEdge> active;
	active.Allocate(total_points);

	for (UInt yi = 0; yi + 1 < ys.GetSize(); ++yi)
	{
		Float y0 = ys[yi];
		Float y1 = ys[yi + 1];
		Float ym = (y0 + y1) * 0.5f;

		active.Clear();

		for (auto & contour : contours)
		{
			if (contour.ymax <= y0 || contour.ymin >= y1) continue;

			REFLEX_LOOP(i, contour.points.GetSize())
			{
				Point a = contour.points[i];
				Point b = contour.points[i + 1 < contour.points.GetSize() ? i + 1 : 0];

				if (a.y == b.y) continue;

				if (!((a.y <= ym && ym < b.y) || (b.y <= ym && ym < a.y))) continue;

				AddScanEdge::Call(active, a, b, y0, ym, y1);
			}
		}

		Sort(active, [](const ScanEdge & a, const ScanEdge & b) 
		{
			return a.x < b.x; 
		});

		output.Allocate(output.GetSize() + ((active.GetSize() >> 1) * 6u));

		if (fill_rule == kFillRuleEvenOdd)
		{
			for (UInt i = 0; i + 1 < active.GetSize(); i += 2)
			{
				Emit::Call(output, y0, y1, active[i], active[i + 1]);
			}
		}
		else
		{
			Int winding = 0;
			Int start = -1;

			REFLEX_LOOP(i, active.GetSize())
			{
				Int next = winding + active[i].winding;

				if (!winding && next)
				{
					start = i;
				}
				else if (winding && !next && start >= 0)
				{
					Emit::Call(output, y0, y1, active[UInt(start)], active[i]);
					start = -1;
				}

				winding = next;
			}
		}
	}
}

REFLEX_END_INTERNAL

void Reflex::GLX::AddRectFill(Points & points, const Rect & rect)
{
	Detail::AddRectFillImpl(points, rect);
}

void Reflex::GLX::AddRectFill(ColourPoints & output, const Colour & colour, const Rect & rect)
{
	Detail::ColourPointsOutput t(output, colour);

	Detail::AddRectFillImpl(t, rect);
}

void Reflex::GLX::AddRectOutline(Points & output, const Rect & rect, const Margin & width)
{
	Detail::AddRectOutlineImpl(output, rect, width);
}

void Reflex::GLX::AddRectOutline(ColourPoints & output, const Colour & colour, const Rect & rect, const Margin & width)
{
	Detail::ColourPointsOutput t(output, colour);

	Detail::AddRectOutlineImpl(t, rect, width);
}

void Reflex::GLX::AddGradientFill(ColourPoints & points, const Colour & from, const Colour & to, const Rect & rect, bool y)
{
	auto origin = rect.origin;

	auto size = rect.size;

	Detail::AddGradientImpl(points, origin.x, origin.y, origin.x + size.w, origin.y + size.h, y, from, to);
}

void Reflex::GLX::AddRoundedFill(Points & output, const Rect & rect, const Corners & corners, Float32 corner_step)
{
	Detail::AllocateExtra(output, 90);

	auto tl = corners.tl;
	auto tr = corners.tr;
	auto br = corners.br;
	auto bl = corners.bl;

	Size tl2 = tl * 2.0f;
	Size tr2 = tr * 2.0f;
	Size bl2 = bl * 2.0f;
	Size br2 = br * 2.0f;

	auto x1 = rect.origin.x;
	auto y1 = rect.origin.y;
	auto x2 = rect.origin.x + rect.size.w;
	auto y2 = rect.origin.y + rect.size.h;

	Rect tlr = { rect.origin, tl2 };
	Rect trr = { { x2 - tr2.w, y1 }, tr2 };
	Rect brr = { { x2 - br2.w, y2 - br2.h }, br2 };
	Rect blr = { { x1, y2 - bl2.h }, bl2 };

	Detail::MakePie(output, tlr, { Detail::kTopLeftCornerOrigin, Detail::kCornerRadians }, corner_step);
	Detail::MakePie(output, trr, { Detail::kTopRightCornerOrigin, Detail::kCornerRadians }, corner_step);
	Detail::MakePie(output, brr, { Detail::kBottomRightCornerOrigin, Detail::kCornerRadians }, corner_step);
	Detail::MakePie(output, blr, { Detail::kBottomLeftCornerOrigin, Detail::kCornerRadians }, corner_step);

	Float midl = x1 + Max(tl.w, bl.w);

	Float midr = x2 - Max(tr.w, br.w);

	Detail::AllocateExtra(output, 5 * 6);

	Detail::AddQuad<kAllocateNone>(output, midl, y1, midr, y2);

	if (tl.w < bl.w)
	{
		Detail::AddQuad<kAllocateNone>(output, x1 + tl.w, y1, midl, y1 + tl.h);
	}
	else
	{
		Detail::AddQuad<kAllocateNone>(output, x1 + bl.w, y2 - bl.h, midl, y2);
	}

	Detail::AddQuad<kAllocateNone>(output, x1, y1 + tl.h, midl, y2 - bl.h);

	if (tr.w < br.w)
	{
		Detail::AddQuad<kAllocateNone>(output, x2 - tr.w, y1, midr, y1 + tr.h);
	}
	else
	{
		Detail::AddQuad<kAllocateNone>(output, x2 - br.w, y2 - br.h, midr, y2);
	}

	Detail::AddQuad<kAllocateNone>(output, midr, y1 + tr.h, x2, y2 - br.h);
}

void Reflex::GLX::AddRoundedOutline(Points & output, const Rect & rect, const Margin & width, const Corners & corners, Float32 corner_step)
{
	auto tl = corners.tl;
	auto tr = corners.tr;
	auto br = corners.br;
	auto bl = corners.bl;

	Size tl2 = tl * 2.0f;
	Size tr2 = tr * 2.0f;
	Size bl2 = bl * 2.0f;
	Size br2 = br * 2.0f;

	auto x1 = rect.origin.x;
	auto y1 = rect.origin.y;
	auto x2 = rect.origin.x + rect.size.w;
	auto y2 = rect.origin.y + rect.size.h;

	Rect tlr = { rect.origin, tl2 };
	Rect trr = { {x2 - tr2.w, y1}, tr2 };
	Rect brr = { {x2 - br2.w, y2 - br2.h}, br2 };
	Rect blr = { {x1, y2 - bl2.h}, bl2 };

	if (auto uniformw = Detail::IsUniform(Reinterpret<Margin>(width)))
	{
		Detail::MakeArc(output, tlr, { Detail::kTopLeftCornerOrigin, Detail::kCornerRadians }, width.near, corner_step);
		Detail::MakeArc(output, trr, { Detail::kTopRightCornerOrigin, Detail::kCornerRadians }, width.near, corner_step);
		Detail::MakeArc(output, brr, { Detail::kBottomRightCornerOrigin, Detail::kCornerRadians }, width.near, corner_step);
		Detail::MakeArc(output, blr, { Detail::kBottomLeftCornerOrigin, Detail::kCornerRadians }, width.near, corner_step);
	}
	else
	{
		Detail::MakeArc(output, tlr, { Detail::kTopLeftCornerOrigin, Detail::kCornerRadians }, { width.near.w, width.near.h }, corner_step);
		Detail::MakeArc(output, trr, { Detail::kTopRightCornerOrigin, Detail::kCornerRadians }, { width.far.w, width.near.h }, corner_step);
		Detail::MakeArc(output, brr, { Detail::kBottomRightCornerOrigin, Detail::kCornerRadians }, { width.far.w, width.far.h }, corner_step);
		Detail::MakeArc(output, blr, { Detail::kBottomLeftCornerOrigin, Detail::kCornerRadians }, { width.near.w, width.far.h, }, corner_step);
	}

	Detail::AllocateExtra(output, 4 * 6);

	Detail::AddQuad<kAllocateNone>(output, x1, y1 + tl.h, x1 + width.near.w, y2 - bl.h);
	Detail::AddQuad<kAllocateNone>(output, x2 - width.far.w, y1 + tr.h, x2, y2 - br.h);
	Detail::AddQuad<kAllocateNone>(output, x1 + tl.w, y1, x2 - tr.w, y1 + width.near.h);
	Detail::AddQuad<kAllocateNone>(output, x1 + bl.w, y2 - width.far.h, x2 - br.w, y2);
}

void Reflex::GLX::AddEllipseOutline(Points & points, const Rect & container, Size width, Float start_radians, Float sweep_radians)
{
	Detail::MakeArc(points, container, Detail::RectifySweep(start_radians, sweep_radians), width, Detail::CalculateCircleStep(container.size));
}

void Reflex::GLX::AddEllipseOutlineRoundCapped(Points & points, const Rect & container, Size width, Float start_radians, Float sweep_radians)
{
	Detail::MakeArc<true>(points, container, Detail::RectifySweep(start_radians, sweep_radians), width, Detail::CalculateCircleStep(container.size));
}

void Reflex::GLX::AddEllipseFill(Points & points, const Rect & rect, Float start_radians, Float sweep_radians)
{
	Detail::MakePie(points, rect, Detail::RectifySweep(start_radians, sweep_radians), Detail::CalculateCircleStep(rect.size));
}

void Reflex::GLX::AddDottedLine(Points & points, Point from, Point to, Size pixel_size)
{
	Point delta = to - from;

	Float dx = delta.x;
	Float dy = delta.y;

	bool y = dy > dx;

	Float d = (&delta.x)[y];

	if (d > 0.0f)
	{
		Float incs[2] = { (dx / d) * pixel_size.w * 2.0f, (dy / d) * pixel_size.h * 2.0f };

		Float end = (&to.x)[y];

		Point point = from;

		auto & pos = (&point.x)[y];

		auto alloc = Truncate((end - pos) / incs[y]) + 2;
			
		auto ptr = Extend(points, alloc).data;

		auto start = ptr;

		while (pos < end)
		{
			*ptr++ = point;

			point += *Reinterpret<Point>(incs);
		}

		auto n = UInt(ptr - start);

		points.Shrink(alloc - n);
	}
}

void Reflex::GLX::Translate(const Points::Region & points, Point offset)
{
	for (auto & i : points)
	{
		i += offset;
	}
}

void Reflex::GLX::Translate(const ColourPoints::Region & points, Point offset)
{
	for (auto & i : points)
	{
		i.a += offset;
	}
}

void Reflex::GLX::Rescale(const Points::Region & points, Scale scale)
{
	for (auto & i : points) i *= scale;
}

void Reflex::GLX::Rescale(const ColourPoints::Region & points, Scale scale)
{
	for (auto & i : points) i.a *= scale;
}

void Reflex::GLX::Rotate(const Points::Region & points, Point origin, Float rotate)
{
	Detail::RotationTransform t(rotate, origin);

	for (auto & i : points) t(i);
}

void Reflex::GLX::Rotate(const ColourPoints::Region & colour_points, Point origin, Float rotate)
{
	Detail::RotationTransform t(rotate, origin);

	for (auto & i : colour_points) t(i);
}

void Reflex::GLX::ModulateColour(const ColourPoints::Region & colour_points, const Colour & colour)
{
	for (auto & i : colour_points)
	{
		i.b = i.b * colour;
	}
}

void Reflex::GLX::AddPolygonFill(Points & output, const Points::View & input)
{
	//TODO cache not thread safe, geometry functions are supposed to work on worker threads

	Detail::Tesselate(g_library->cache.int_workspace, input, output);
}

void Reflex::GLX::AddPolygonFill(Points & output, const ArrayView <Points::View> & input, FillRule fill_rule)
{
	//TODO cache not thread safe, geometry functions are supposed to work on worker threads

	Detail::AddCompoundPolygonFillImpl(g_library->cache.int_workspace, output, input, fill_rule);
}

void Reflex::GLX::AddPointsWithColour(ColourPoints & output, const Points::View & input, const Colour & colour)
{
	auto poutput = Extend(output, input.size).data;

	for (auto & i : input)
	{
		*poutput++ = { i, colour };
	}
}

//void Reflex::GLX::AddPointsWithGradient(ColourPoints & points, const Colour & from, const Colour & to, bool y, const Points::View & input, const Rect & rect)
//{
//	auto p = (&rect.origin.x) + y;
//
//	auto offset = *p;
//
//	auto invrange = Reflex::Reciprocal(Max(p[2], 1.0f));
//
//	auto poutput = Extend(points, input.size).data;
//
//	REFLEX_FOREACH(point, input)
//	{
//		auto pos = Reinterpret<Float>(&point.x);
//
//		(*poutput++) = { point, LinearInterpolate((pos[y] - offset) * invrange, from, to) };
//	}
//}

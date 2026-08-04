#include "reflex/glx/detail/svg.h"
#include "reflex/glx/detail/functions/misc.h"




REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

constexpr CString::View kSVG = "svg";

enum SvgTransformType : UInt8
{
	kSvgTransformTranslate,
	kSvgTransformScale,
	kSvgTransformRotate,
	kSvgTransformMatrix,
};

struct SvgMatrix
{
	Float values[6] = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
};

struct SvgStyle
{
	PathCap line_cap = kPathCapButt;
	PathJoin line_join = kPathJoinMiter;
	FillRule fill_rule = kFillRuleNonZero;
	Float32 stroke_width = 1.0f;
	Colour fill_colour = kBlack;
	Colour stroke_colour = kTransparent;
	Float32 miter_limit = 4.0f;
};

struct SvgState
{
	Size render_size;
	Float corner_step = kCornerStepRadians;

	SvgMatrix matrix;
	SvgStyle style;

	Points fill_workspace, stroke_workspace;

	Array <Points::View> contours_workspace;

	ColourPoints & output;
};

struct SvgSubPath
{
	Points points;
	bool closed = false;
};

struct SvgLength
{
	Float value = 0.0f;
	bool valid = false;
	bool percent = false;
};

bool IsSvgWs(char value)
{
	return value == ' ' || value == '\t' || value == '\n' || value == '\r' || value == ',';
}

bool IsSvgDigit(char c)
{
	return c >= '0' && c <= '9';
}

void SkipSvgWs(CString::View & itr)
{
	Data::Detail::IterateWhile<IsSvgWs>(itr);
}

bool ReadSvgFloat(CString::View & itr, Float & value)
{
	SkipSvgWs(itr);

	if (!itr)
	{
		return false;
	}

	auto first = itr.data;
	UInt32 consumed = 0;
	bool has_digits = false;
	UInt32 exponent_offset = 0;

	if (*itr.data == '-' || *itr.data == '+')
	{
		++consumed;
	}

	while (consumed < itr.size && IsSvgDigit(first[consumed]))
	{
		has_digits = true;
		++consumed;
	}

	if (consumed < itr.size && first[consumed] == '.')
	{
		++consumed;

		while (consumed < itr.size && IsSvgDigit(first[consumed]))
		{
			has_digits = true;
			++consumed;
		}
	}

	if (!has_digits)
	{
		return false;
	}

	if (consumed < itr.size && (first[consumed] == 'e' || first[consumed] == 'E'))
	{
		auto exp = consumed + 1;

		if (exp < itr.size && (first[exp] == '-' || first[exp] == '+'))
		{
			++exp;
		}

		auto exp_digits = exp;
		while (exp < itr.size && IsSvgDigit(first[exp]))
		{
			++exp;
		}

		if (exp > exp_digits)
		{
			exponent_offset = consumed;
			consumed = exp;
		}
	}

	CString::View number = { first, consumed };

	itr.data += consumed;
	itr.size -= consumed;

	if (exponent_offset)
	{
		auto base = Mid(number, 0, exponent_offset);
		auto exponent = Mid(number, exponent_offset + 1);

		value = ToFloat32(base) * Pow(10.0f, ToFloat32(exponent));
	}
	else
	{
		value = ToFloat32(number);
	}

	return true;
}

Pair <Float,bool> ReadSvgLength(CString::View value)
{
	Pair <Float,bool> result;

	if (ReadSvgFloat(value, result.a))
	{
		result.b = *value.data == '%';
	}

	return result;
}

bool ReadSvgViewBox(const Data::PropertySet & node, Rect & rect)
{
	if (auto value = Data::GetCString(node, K32("viewBox")))
	{
		auto itr = value;

		if (ReadSvgFloat(itr, rect.origin.x) && ReadSvgFloat(itr, rect.origin.y) && ReadSvgFloat(itr, rect.size.w) && ReadSvgFloat(itr, rect.size.h))
		{
			return rect.size.w > 0.0f && rect.size.h > 0.0f;
		}
	}

	return false;
}

void ParseSvgPreserveAspectRatio(const Data::PropertySet & node, SVG & svg)
{
	constexpr Key32 kAligns[] = { K32("xMinYMin"), K32("xMidYMin"), K32("xMaxYMin"), K32("xMinYMid"), K32("xMidYMid"), K32("xMaxYMid"), K32("xMinYMax"), K32("xMidYMax"), K32("xMaxYMax") };

	constexpr Pair <Orientation> kAlignToOrientation[] =
	{
		{ kOrientationNear, kOrientationNear }, // xMinYMin
		{ kOrientationCenter, kOrientationNear }, // xMidYMin
		{ kOrientationFar, kOrientationNear }, // xMaxYMin
		{ kOrientationNear, kOrientationCenter }, // xMinYMid
		{ kOrientationCenter, kOrientationCenter }, // xMidYMid
		{ kOrientationFar, kOrientationCenter }, // xMaxYMid
		{ kOrientationNear, kOrientationFar }, // xMinYMax
		{ kOrientationCenter, kOrientationFar }, // xMidYMax
		{ kOrientationFar, kOrientationFar }  // xMaxYMax
	};

	constexpr Key32 kFits[] = { K32(""), K32("none"), K32("meet"), K32("slice"), };

	if (auto itr = Data::GetCString(node, K32("preserveAspectRatio")))
	{
		auto split = Search(itr, ' ');

		auto align = Key32(Left<true>(itr, split.value));

		if (align == K32("none"))
		{
			svg.fit = kFitModeStretch;
		}
		else
		{
			auto [x, y] = kAlignToOrientation[ParseEnum(align, kAligns, UInt8(4))];

			svg.orientation.a = x;
			svg.orientation.b = y;

			if (split)
			{
				svg.fit = ParseEnum<FitMode>(Mid(itr, split.value + 1), kFits, svg.fit);
			}
		}
	}
}

SVG InspectSvgNode(const Data::PropertySet & node)
{
	SVG svg =
	{
		.id = Data::GetCString(node, K32("id")),
		.size = {},
		.size_normalized = {},
		.fit = kFitModeContain,
		.viewport = {},
		.orientation = { kOrientationCenter, kOrientationCenter },
		.node = node
	};

	auto [width,width_percent] = ReadSvgLength(Data::GetCString(node, K32("width")));
	auto [height,height_percent] = ReadSvgLength(Data::GetCString(node, K32("height")));

	svg.size.w = width_percent ? (width * 0.01f) : width;
	svg.size.h = height_percent ? (height * 0.01f) : height;

	svg.size_normalized.a = width_percent;
	svg.size_normalized.b = height_percent;

	ParseSvgPreserveAspectRatio(node, svg);

	if (ReadSvgViewBox(node, svg.viewport))
	{
		if (svg.size.w > 0.0f && svg.size.h <= 0.0f && !svg.size_normalized.a)
		{
			svg.size.h = svg.size.w * (svg.viewport.size.h / svg.viewport.size.w);
		}
		else if (svg.size.h > 0.0f && svg.size.w <= 0.0f && !svg.size_normalized.b)
		{
			svg.size.w = svg.size.h * (svg.viewport.size.w / svg.viewport.size.h);
		}
		else if (svg.size.w <= 0.0f && svg.size.h <= 0.0f)
		{
			svg.size = svg.viewport.size;
		}
	}
	else
	{
		svg.viewport = { kOrigin, { svg.size_normalized.a ? 0.0f : svg.size.w, svg.size_normalized.b ? 0.0f : svg.size.h } };
	}

	return svg;
}

Points GetSvgPointsProperty(const Data::PropertySet & node, Key32 id = K32("points"))
{
	auto value = Data::GetCString(node, id);

	Points points;

	auto itr = value;

	Point xy;

	while (ReadSvgFloat(itr, xy.x))
	{
		ReadSvgFloat(itr, xy.y);

		points.Push(xy);
	}

	return points;
}

REFLEX_INLINE void MulMatrix(Float out[6], const Float A[6], const Float B[6])
{
	// SVG affine:
	// [ a c e ]
	// [ b d f ]
	// [ 0 0 1 ]
	//
	// array layout: [a b c d e f]

	const Float a = A[0] * B[0] + A[2] * B[1];
	const Float b = A[1] * B[0] + A[3] * B[1];

	const Float c = A[0] * B[2] + A[2] * B[3];
	const Float d = A[1] * B[2] + A[3] * B[3];

	const Float e = A[0] * B[4] + A[2] * B[5] + A[4];
	const Float f = A[1] * B[4] + A[3] * B[5] + A[5];

	out[0] = a; out[1] = b;
	out[2] = c; out[3] = d;
	out[4] = e; out[5] = f;
}

REFLEX_INLINE void SetIdentity(Float m[6])
{
	m[0] = 1.0f; m[1] = 0.0f;
	m[2] = 0.0f; m[3] = 1.0f;
	m[4] = 0.0f; m[5] = 0.0f;
}

REFLEX_INLINE void MakeTranslate(Float m[6], Float tx, Float ty)
{
	SetIdentity(m);
	m[4] = tx;
	m[5] = ty;
}

REFLEX_INLINE void MakeScale(Float m[6], Float sx, Float sy)
{
	SetIdentity(m);
	m[0] = sx;
	m[3] = sy;
}

REFLEX_INLINE void MakeRotate(Float m[6], Float radians)
{
	const Float c = Float(std::cos(radians));
	const Float s = Float(std::sin(radians));

	// [ c -s 0 ]
	// [ s  c 0 ]
	SetIdentity(m);
	m[0] = c;
	m[1] = s;
	m[2] = -s;
	m[3] = c;
}

REFLEX_INLINE void MakeMatrix(Float m[6], const Float v[6])
{
	m[0] = v[0]; m[1] = v[1];
	m[2] = v[2]; m[3] = v[3];
	m[4] = v[4]; m[5] = v[5];
}

REFLEX_INLINE void ParseSvgTransform(const Data::PropertySet & node, SvgMatrix & matrix)
{
	constexpr auto ReadWord = [](CString::View & itr)
	{
		SkipSvgWs(itr);
		
		return Data::Detail::ExtractWhile<Data::Detail::IsAlphaCharacter>(itr);
	};

	if (auto itr = Data::GetCString(node, "transform"))
	{
		while (auto name = ReadWord(itr))
		{
			SkipSvgWs(itr);

			if (!itr || *itr.data != '(') break;

			++itr.data;
			--itr.size;

			Float vals[6] = { 0, 0, 0, 0, 0, 0 };

			UInt n = 0;
			while (itr.size && *itr.data != ')' && n < 6)
			{
				if (!ReadSvgFloat(itr, vals[n++])) break;
			}

			if (itr && *itr.data == ')')
			{
				++itr.data;
				--itr.size;
			}

			Float local[6];
			SetIdentity(local);

			switch (MakeKey32(name))
			{
			case K32("translate"):
			{
				const Float tx = (n > 0) ? vals[0] : 0.0f;
				const Float ty = (n > 1) ? vals[1] : 0.0f;
				MakeTranslate(local, tx, ty);
				break;
			}
			case K32("scale"):
			{
				const Float sx = (n > 0) ? vals[0] : 1.0f;
				const Float sy = (n > 1) ? vals[1] : sx;
				MakeScale(local, sx, sy);
				break;
			}
			case K32("rotate"):
			{
				const Float deg = (n > 0) ? vals[0] : 0.0f;
				const Float rad = deg * (kPif / 180.0f);

				const Float cx = (n > 2) ? vals[1] : 0.0f;
				const Float cy = (n > 2) ? vals[2] : 0.0f;

				Float R[6]; MakeRotate(R, rad);

				if (n > 2)
				{
					Float T0[6]; MakeTranslate(T0, -cx, -cy);
					Float T1[6]; MakeTranslate(T1, cx, cy);

					Float tmp[6];
					MulMatrix(tmp, R, T0);
					MulMatrix(local, T1, tmp);
				}
				else
				{
					MakeMatrix(local, R);
				}
				break;
			}
			case K32("matrix"):
			{
				if (n < 6) return;
				MakeMatrix(local, vals);
				break;
			}
			default:
				return;
			}

			Float composed[6];
			MulMatrix(composed, matrix.values, local);
			MakeMatrix(matrix.values, composed);
		}
	}
}

REFLEX_INLINE void ApplyTransform(const SvgMatrix & t, const Points::Region & region)
{
	const Float a = t.values[0];
	const Float b = t.values[1];
	const Float c = t.values[2];
	const Float d = t.values[3];
	const Float e = t.values[4];
	const Float f = t.values[5];

	for (auto & p : region)
	{
		auto v = p;
		p = { a * v.x + c * v.y + e, b * v.x + d * v.y + f };
	}
}

REFLEX_NOINLINE void GetSvgFloatProperties(const Data::PropertySet & node, ArrayView <Key32> ids, ArrayRegion <Float> buffer)
{
	REFLEX_ASSERT(ids.size == buffer.size);
		
	auto ptr = buffer.data;

	for (auto id : ids)
	{
		*ptr++ = ToFloat32(GetProperty<Data::CStringProperty>(node, id)->value);
	}
}

Point ReflectControlPoint(Point point, Point control)
{
	return { point.x + (point.x - control.x), point.y + (point.y - control.y) };
}

UInt CalculateNumBezierSubdivisions(Float curve_step, Size render_size, Float radius_hint = 0.0f)
{
	Float normalised = Max(0.0001f, curve_step);
	Float max_dim = Max(render_size.w, render_size.h);
	Float radius = Max(radius_hint, 1.0f);
	Float scale = Max(max_dim / 24.0f, 1.0f);
	Float target_seg = (radius * scale) / Max(normalised * 1.25f, 0.05f);
	UInt segments = UInt(target_seg);
	return Clip(segments, 16u, 512u);
}

REFLEX_INLINE Point EvaluateQuadraticBezier(Point p0, Point p1, Point p2, Float t)
{
	Float u = 1.0f - t;
	Float uu = u * u;
	Float tt = t * t;
	return { (uu * p0.x) + (2.0f * u * t * p1.x) + (tt * p2.x), (uu * p0.y) + (2.0f * u * t * p1.y) + (tt * p2.y) };
}

void AddQuadraticBezierPoints(Points & points, Point p0, Point p1, Point p2, Float curve_step, Size render_size)
{
	UInt num_segments = CalculateNumBezierSubdivisions(curve_step, render_size);

	auto segments = Extend(points, num_segments);

	Float step = 1.0f / Float(num_segments);
	Float t = step;

	for (auto & i : segments)
	{
		i = EvaluateQuadraticBezier(p0, p1, p2, t);
		t += step;
	}
}

REFLEX_INLINE Point EvaluateCubicBezier(Point p0, Point p1, Point p2, Point p3, Float t)
{
	Float u = 1.0f - t;
	Float uu = u * u;
	Float uuu = uu * u;
	Float tt = t * t;
	Float ttt = tt * t;
	return { (uuu * p0.x) + (3.0f * uu * t * p1.x) + (3.0f * u * tt * p2.x) + (ttt * p3.x), (uuu * p0.y) + (3.0f * uu * t * p1.y) + (3.0f * u * tt * p2.y) + (ttt * p3.y) };
}

void AddCubicBezierPoints(Points & points, Point p0, Point p1, Point p2, Point p3, Float curve_step, Size render_size)
{
	UInt num_segments = CalculateNumBezierSubdivisions(curve_step, render_size);

	auto segments = Extend(points, num_segments);

	Float step = 1.0f / Float(num_segments);
	Float t = step;

	for (auto & i : segments)
	{
		i = EvaluateCubicBezier(p0, p1, p2, p3, t);
		t += step;
	}
}

void AddArcPoints(Points & points, Point from, Float rx, Float ry, Float x_axis_rotation_deg, bool large_arc, bool sweep, Point to, Float curve_step, Size render_size)
{
	rx = Abs(rx);
	ry = Abs(ry);

	if (rx < 0.00001f || ry < 0.00001f)
	{
		points.Push(to);
		return;
	}

	Float phi = x_axis_rotation_deg * (kPif / 180.0f);
	Float cos_phi = Float(std::cos(phi));
	Float sin_phi = Float(std::sin(phi));

	Float dx2 = (from.x - to.x) * 0.5f;
	Float dy2 = (from.y - to.y) * 0.5f;

	Float x1p = (cos_phi * dx2) + (sin_phi * dy2);
	Float y1p = (-sin_phi * dx2) + (cos_phi * dy2);

	Float rx2 = rx * rx;
	Float ry2 = ry * ry;
	Float x1p2 = x1p * x1p;
	Float y1p2 = y1p * y1p;

	Float lambda = (x1p2 / rx2) + (y1p2 / ry2);
	if (lambda > 1.0f)
	{
		Float s = Float(std::sqrt(lambda));
		rx *= s;
		ry *= s;
		rx2 = rx * rx;
		ry2 = ry * ry;
	}

	Float sign = (large_arc == sweep) ? -1.0f : 1.0f;
	Float nom = (rx2 * ry2) - (rx2 * y1p2) - (ry2 * x1p2);
	Float den = (rx2 * y1p2) + (ry2 * x1p2);
	Float coef = 0.0f;
	if (den > 0.0f)
	{
		coef = sign * Float(std::sqrt(Max(0.0f, nom / den)));
	}

	Float cxp = coef * ((rx * y1p) / ry);
	Float cyp = coef * (-(ry * x1p) / rx);

	Float cx = (cos_phi * cxp) - (sin_phi * cyp) + ((from.x + to.x) * 0.5f);
	Float cy = (sin_phi * cxp) + (cos_phi * cyp) + ((from.y + to.y) * 0.5f);

	constexpr auto vector_angle = [](Float ux, Float uy, Float vx, Float vy)
	{
		Float dot = (ux * vx) + (uy * vy);
		Float det = (ux * vy) - (uy * vx);
		return Float(std::atan2(det, dot));
	};

	Float ux = (x1p - cxp) / rx;
	Float uy = (y1p - cyp) / ry;
	Float vx = (-x1p - cxp) / rx;
	Float vy = (-y1p - cyp) / ry;

	Float theta1 = vector_angle(1.0f, 0.0f, ux, uy);
	Float dtheta = vector_angle(ux, uy, vx, vy);

	if (!sweep && dtheta > 0.0f) dtheta -= k2Pif;
	if (sweep && dtheta < 0.0f) dtheta += k2Pif;

	UInt num_segments = UInt((Abs(dtheta) / k2Pif) * Float(CalculateNumBezierSubdivisions(curve_step, render_size, Max(rx, ry))));
	num_segments = Clip(num_segments, 6u, 160u);

	auto segments = Extend(points, num_segments);

	Float theta_step = dtheta / Float(num_segments);
	Float theta = theta1 + theta_step;

	for (auto & i : segments)
	{
		Float c = Cos(theta);
		Float s = Sin(theta);
		i = { cx + (rx * cos_phi * c) - (ry * sin_phi * s), cy + (rx * sin_phi * c) + (ry * cos_phi * s) };
		theta += theta_step;
	}
}

Array <SvgSubPath> ReadSvgPath(SvgState & state, const CString::View & value, Float curve_step, Size render_size)
{
	constexpr auto ReadSvgFloats = [](CString::View & itr, ArrayRegion <Float> buffer) -> ArrayView <Float>
	{
		REFLEX_LOOP(i, buffer.size)
		{
			if (!ReadSvgFloat(itr, buffer[i]))
			{
				return {};
			}
		}

		return { buffer.data, buffer.size };
	};

	Array <SvgSubPath> subpaths;

	auto itr = value;

	Point current = {};
	Point start = {};
	Point prev_cubic_control = {};
	Point prev_quad_control = {};
	bool has_prev_cubic_control = false;
	bool has_prev_quad_control = false;
	char command = 0;
	Float buffer[7] = {};

	auto append_point = [&](Point point)
	{
		if (!subpaths.GetSize())
		{
			subpaths.Push({});
		}

		subpaths[subpaths.GetSize() - 1].points.Push(point);
	};

	auto & workspace = state.stroke_workspace;

	while (itr.size)
	{
		SkipSvgWs(itr);
		
		if (!itr) break;

		if (Data::Detail::IsAlphaCharacter(*itr.data))
		{
			command = *itr.data++;
			--itr.size;

			if (command == 'Z' || command == 'z')
			{
				if (subpaths.GetSize())
				{
					auto & subpath = subpaths[subpaths.GetSize() - 1];
					subpath.closed = true;

					auto n = subpath.points.GetSize();
					if (n && !(subpath.points[n - 1] == start))
					{
						subpath.points.Push(start);
					}
				}
				current = start;
				has_prev_cubic_control = false;
				has_prev_quad_control = false;
				command = 0;
				continue;
			}
		}

		workspace.Clear();

		switch (command)
		{
		case 'M':
		case 'L':
			if (auto values = ReadSvgFloats(itr, { buffer, 2 }))
			{
				current = { values[0], values[1] };
				if (command == 'M')
				{
					subpaths.Push({});
					start = current;
					command = 'L';
				}
				append_point(current);
				has_prev_cubic_control = false;
				has_prev_quad_control = false;
			}
			break;

		case 'm':
		case 'l':
			if (auto values = ReadSvgFloats(itr, { buffer, 2 }))
			{
				current = current + Point{ values[0], values[1] };
				if (command == 'm')
				{
					subpaths.Push({});
					start = current;
					command = 'l';
				}
				append_point(current);
				has_prev_cubic_control = false;
				has_prev_quad_control = false;
			}
			break;

		case 'H':
		case 'h':
		case 'V':
		case 'v':
			if (auto values = ReadSvgFloats(itr, { buffer, 1 }))
			{
				if (command == 'H') current.x = values[0];
				else if (command == 'h') current.x += values[0];
				else if (command == 'V') current.y = values[0];
				else current.y += values[0];
				append_point(current);
				has_prev_cubic_control = false;
				has_prev_quad_control = false;
			}
			break;

		case 'C':
			if (auto values = ReadSvgFloats(itr, { buffer, 6 }))
			{
				AddCubicBezierPoints(workspace, current, { values[0], values[1] }, { values[2], values[3] }, { values[4], values[5] }, curve_step, render_size);
				for (auto & pt : workspace) append_point(pt);
				current = { values[4], values[5] };
				prev_cubic_control = { values[2], values[3] };
				has_prev_cubic_control = true;
				has_prev_quad_control = false;
			}
			break;

		case 'c':
			if (auto values = ReadSvgFloats(itr, { buffer, 6 }))
			{
				Point c1 = current + Point{ values[0], values[1] };
				Point c2 = current + Point{ values[2], values[3] };
				Point next = current + Point{ values[4], values[5] };
				AddCubicBezierPoints(workspace, current, c1, c2, next, curve_step, render_size);
				for (auto & pt : workspace) append_point(pt);
				current = next;
				prev_cubic_control = c2;
				has_prev_cubic_control = true;
				has_prev_quad_control = false;
			}
			break;

		case 'S':
			if (auto values = ReadSvgFloats(itr, { buffer, 4 }))
			{
				Point c1 = has_prev_cubic_control ? ReflectControlPoint(current, prev_cubic_control) : current;
				Point c2 = { values[0], values[1] };
				Point next = { values[2], values[3] };
				AddCubicBezierPoints(workspace, current, c1, c2, next, curve_step, render_size);
				for (auto & pt : workspace) append_point(pt);
				current = next;
				prev_cubic_control = c2;
				has_prev_cubic_control = true;
				has_prev_quad_control = false;
			}
			break;

		case 's':
			if (auto values = ReadSvgFloats(itr, { buffer, 4 }))
			{
				Point c1 = has_prev_cubic_control ? ReflectControlPoint(current, prev_cubic_control) : current;
				Point c2 = current + Point{ values[0], values[1] };
				Point next = current + Point{ values[2], values[3] };
				AddCubicBezierPoints(workspace, current, c1, c2, next, curve_step, render_size);
				for (auto & pt : workspace) append_point(pt);
				current = next;
				prev_cubic_control = c2;
				has_prev_cubic_control = true;
				has_prev_quad_control = false;
			}
			break;

		case 'Q':
			if (auto values = ReadSvgFloats(itr, { buffer, 4 }))
			{
				AddQuadraticBezierPoints(workspace, current, { values[0], values[1] }, { values[2], values[3] }, curve_step, render_size);
				for (auto & pt : workspace) append_point(pt);
				current = { values[2], values[3] };
				prev_quad_control = { values[0], values[1] };
				has_prev_quad_control = true;
				has_prev_cubic_control = false;
			}
			break;

		case 'q':
			if (auto values = ReadSvgFloats(itr, { buffer, 4 }))
			{
				Point control = current + Point{ values[0], values[1] };
				Point next = current + Point{ values[2], values[3] };
				AddQuadraticBezierPoints(workspace, current, control, next, curve_step, render_size);
				for (auto & pt : workspace) append_point(pt);
				prev_quad_control = control;
				current = next;
				has_prev_quad_control = true;
				has_prev_cubic_control = false;
			}
			break;

		case 'T':
			if (auto values = ReadSvgFloats(itr, { buffer, 2 }))
			{
				Point control = has_prev_quad_control ? ReflectControlPoint(current, prev_quad_control) : current;
				Point next = { values[0], values[1] };
				AddQuadraticBezierPoints(workspace, current, control, next, curve_step, render_size);
				for (auto & pt : workspace) append_point(pt);
				current = next;
				prev_quad_control = control;
				has_prev_quad_control = true;
				has_prev_cubic_control = false;
			}
			break;

		case 't':
			if (auto values = ReadSvgFloats(itr, { buffer, 2 }))
			{
				Point control = has_prev_quad_control ? ReflectControlPoint(current, prev_quad_control) : current;
				Point next = current + Point{ values[0], values[1] };
				AddQuadraticBezierPoints(workspace, current, control, next, curve_step, render_size);
				for (auto & pt : workspace) append_point(pt);
				current = next;
				prev_quad_control = control;
				has_prev_quad_control = true;
				has_prev_cubic_control = false;
			}
			break;

		case 'A':
			if (auto values = ReadSvgFloats(itr, { buffer, 7 }))
			{
				AddArcPoints(workspace, current, values[0], values[1], values[2], values[3] != 0.0f, values[4] != 0.0f, { values[5], values[6] }, curve_step, render_size);
				for (auto & pt : workspace) append_point(pt);
				current = { values[5], values[6] };
				has_prev_quad_control = false;
				has_prev_cubic_control = false;
			}
			break;

		case 'a':
			if (auto values = ReadSvgFloats(itr, { buffer, 7 }))
			{
				Point next = current + Point{ values[5], values[6] };
				AddArcPoints(workspace, current, values[0], values[1], values[2], values[3] != 0.0f, values[4] != 0.0f, next, curve_step, render_size);
				for (auto & pt : workspace) append_point(pt);
				current = next;
				has_prev_quad_control = false;
				has_prev_cubic_control = false;
			}
			break;

		default:
			break;
		}
	}

	workspace.Clear();

	return subpaths;
}

constexpr Pair<Key32, Quad<UInt8>> kColourConstants[] =
{
	{ K32("none"), { 0, 0, 0, 0 } },

	{ K32("black"), { 0, 0, 0, 255 } },
	{ K32("white"), { 255, 255, 255, 255 } },

	{ K32("silver"), { 192, 192, 192, 255 } },
	{ K32("gray"), { 128, 128, 128, 255 } },

	{ K32("maroon"), { 128, 0, 0, 255 } },
	{ K32("red"), { 255, 0, 0, 255 } },
	{ K32("purple"), { 128, 0, 128, 255 } },
	{ K32("fuchsia"), { 255, 0, 255, 255 } },

	{ K32("green"), { 0, 128, 0, 255 } },
	{ K32("lime"), { 0, 255, 0, 255 } },
	{ K32("olive"), { 128, 128, 0, 255 } },
	{ K32("yellow"), { 255, 255, 0, 255 } },

	{ K32("navy"), { 0, 0, 128, 255 } },
	{ K32("blue"), { 0, 0, 255, 255 } },
	{ K32("teal"), { 0, 128, 128, 255 } },
	{ K32("aqua"), { 0, 255, 255, 255 } }
};

void ParseSvgColour(Colour & out, CString::View string)
{
	if (string)
	{
		if (string[0] == '#')
		{
			out = Detail::ToColour(Mid(string, 1), out);
		}
		else if (auto rgb = Search(string, '('))
		{
			string = Mid(string, rgb.value);

			if (string.size) string.size--;

			auto values = Split(string, ',');

			auto ptr = &out.r;

			REFLEX_LOOP(idx, Min<UInt>(values.GetSize(), 4))
			{
				*ptr++ = ToFloat32(values[idx]) / 255.0f;
			}
		}
		else if (auto constant = SearchValue<KeyCompare>(ToView(kColourConstants), Key32(string)))
		{
			auto rgba = constant->b;

			out = RGB(rgba.a, rgba.b, rgba.c, rgba.d);
		}
	}
}

REFLEX_INLINE void ParseNodeStyle(const Data::PropertySet & node, SvgStyle & style)
{
	constexpr auto ApplyStyleProperty = [](SvgStyle & style, Key32 key, CString::View value)
	{
		constexpr Key32 kLineCaps[] = { K32("butt"), K32("round"), K32("square") };
		constexpr Key32 kLineJoins[] = { K32("miter"), K32("round"), K32("bevel") };
		constexpr Key32 kFillRules[] = { K32("nonzero"), K32("evenodd") };

		switch (key.value)
		{
		case K32("fill"):
			ParseSvgColour(style.fill_colour, value);
			break;

		case K32("stroke"):
			ParseSvgColour(style.stroke_colour, value);
			break;

		case K32("stroke-width"):
			style.stroke_width = ToFloat32(value);
			break;

		case K32("stroke-linecap"):
			style.line_cap = Detail::ParseEnum(value, kLineCaps, style.line_cap);
			break;

		case K32("stroke-linejoin"):
			style.line_join = Detail::ParseEnum(value, kLineJoins, style.line_join);
			break;

		case K32("stroke-miterlimit"):
			style.miter_limit = ToFloat32(value);
			break;

		case K32("fill-rule"):
			style.fill_rule = Detail::ParseEnum(value, kFillRules, style.fill_rule);
			break;

		case K32("opacity"):
			style.fill_colour.a = ToFloat32(value);
			style.stroke_colour.a = ToFloat32(value);
			break;

		case K32("fill-opacity"):
			style.fill_colour.a = ToFloat32(value);
			break;

		case K32("stroke-opacity"):
			style.stroke_colour.a = ToFloat32(value);
			break;

		default:
			break;
		}
	};

	for (auto & [adr, value] : node.Iterate<Data::CStringProperty>())
	{
		ApplyStyleProperty(style, adr.id, value->value);
	}

	if (auto css = Data::GetCString(node, K32("style")))
	{
		for (auto & i : Split(css, ';'))
		{
			auto split = Search(i, ':');

			auto [key, value] = Splice<true>(i, split.value);

			value = Mid<true>(value, 1);

			ApplyStyleProperty(style, Key32(key), value);
		}
	}
}

void AddSvgRect(SvgState & state, Points & fill, Points & stroke, const Data::PropertySet & node)
{
	constexpr Key32 ids[] = { K32("x"), K32("y"), K32("width"), K32("height"), K32("rx"), K32("ry") };
	Float buffer[6];
	GetSvgFloatProperties(node, ToView(ids), ToRegion(buffer));

	auto rx = buffer[4];
	auto ry = buffer[5];

	Rect rect = { { buffer[0], buffer[1] }, { buffer[2], buffer[3] } };

	const auto & style = state.style;

	if (rx > 0.0f && ry > 0.0f)
	{
		auto radius = Min(rx, ry);

		if (style.fill_colour.a)
		{
			AddRoundedFill(fill, rect, radius, state.corner_step);
		}

		if (style.stroke_colour.a)
		{
			AddRoundedOutline(stroke, rect, MakeMargin(style.stroke_width * 0.5f), radius, state.corner_step);
		}
	}
	else
	{
		if (style.fill_colour.a)
		{
			AddRectFill(fill, rect);
		}

		if (style.stroke_colour.a)
		{
			AddRectOutline(stroke, rect, MakeMargin(style.stroke_width));
		}
	}
}

void AddSvgEllipseImpl(SvgState & state, Points & fill, Points & stroke, const Rect & rect)
{
	auto & style = state.style;

	if (style.fill_colour.a)
	{
		AddEllipseFill(fill, rect);
	}

	if (style.stroke_colour.a)
	{
		AddEllipseOutline(stroke, rect, Reflex::MakeSize(state.style.stroke_width));
	}
}

void AddSvgCircle(SvgState & state, Points & fill, Points & stroke, const Data::PropertySet & node)
{
	constexpr Key32 ids[] = { K32("cx"), K32("cy"), K32("r") };
	Float buffer[3];
	GetSvgFloatProperties(node, ToView(ids), ToRegion(buffer));

	auto cx = buffer[0];
	auto cy = buffer[1];
	auto r = buffer[2];

	AddSvgEllipseImpl(state, fill, stroke, { { cx - r, cy - r }, { r * 2.0f, r * 2.0f } });
}

void AddSvgEllipse(SvgState & state, Points & fill, Points & stroke, const Data::PropertySet & node)
{
	constexpr Key32 ids[] = { K32("cx"), K32("cy"), K32("rx"), K32("ry") };
	Float buffer[4];
	GetSvgFloatProperties(node, ToView(ids), ToRegion(buffer));

	auto cx = buffer[0];
	auto cy = buffer[1];
	auto rx = buffer[2];
	auto ry = buffer[3];

	AddSvgEllipseImpl(state, fill, stroke, { { cx - rx, cy - ry }, { rx * 2.0f, ry * 2.0f } });
}

void DispatchSvgPath(Points & stroke, const Points::View & path, bool closed, const SvgStyle & style)
{
	if (style.line_cap || style.line_join)
	{
		AddPath(stroke, path, closed, style.stroke_width, style.line_join, style.line_cap, style.miter_limit);
	}
	else
	{
		AddPath(stroke, path, closed, style.stroke_width);
	}
}

void AddSvgLine(SvgState & state, Points & fill, Points & stroke, const Data::PropertySet & node)
{
	constexpr Key32 ids[] = { K32("x1"), K32("y1"), K32("x2"), K32("y2") };
	Point path[2];
	GetSvgFloatProperties(node, ToView(ids), { &path[0].x, 4 });

	DispatchSvgPath(stroke, path, false, state.style);
}

void AddSvgPolyLine(SvgState & state, Points & fill, Points & stroke, const Data::PropertySet & node)
{
	auto points = GetSvgPointsProperty(node);

	DispatchSvgPath(stroke, points, false, state.style);
}

void AddSvgPath(SvgState & state, Points & fill, Points & stroke, const Data::PropertySet & node)
{
	auto & style = state.style;

	auto subpaths = ReadSvgPath(state, Data::GetCString(node, K32("d")), state.corner_step, state.render_size);

	if (style.fill_colour.a)
	{
		auto & contours = state.contours_workspace;
		
		contours.Clear();

		for (auto & i : subpaths)
		{
			if (i.closed && i.points.GetSize() > 2)
			{
				auto n = i.points.GetSize();
				while (n > 3 && i.points[n - 1] == i.points[0]) --n;

				contours.Push({ i.points.GetData(), n });
			}
		}

		AddPolygonFill(fill, contours, style.fill_rule);
	}

	if (style.stroke_colour.a)
	{
		for (auto & i : subpaths)
		{
			DispatchSvgPath(stroke, i.points, i.closed, style);
		}
	}

}

void AddSvgPolygon(SvgState & state, Points & fill, Points & stroke, const Data::PropertySet & node)
{
	auto & style = state.style;

	auto points = GetSvgPointsProperty(node);

	if (style.fill_colour.a)
	{
		auto & contours = state.contours_workspace;

		contours.Clear();
		
		contours.Push({ points.GetData(), points.GetSize() });

		AddPolygonFill(fill, contours, style.fill_rule);
	}

	if (style.stroke_colour.a)
	{
		AddPath(stroke, points, true, style.stroke_width);
	}
}

void AppendSvgNode(SvgState & state, const Data::PropertySet & node)
{
	auto & fill = state.fill_workspace;
	auto & stroke = state.stroke_workspace;

	fill.Clear();
	stroke.Clear();


	//push
	auto & style = state.style;
	auto style_z = style;
	ParseNodeStyle(node, style);

	auto & matrix = state.matrix;;
	auto transform_z = matrix;
	ParseSvgTransform(node, matrix);


	//dispatch tag
	switch (MakeKey32(Data::GetXmlTag(node)))
	{
	case K32("path"):
		AddSvgPath(state, fill, stroke, node);
		break;

	case K32("rect"):
		AddSvgRect(state, fill, stroke, node);
		break;

	case K32("circle"):
		AddSvgCircle(state, fill, stroke, node);
		break;

	case K32("ellipse"):
		AddSvgEllipse(state, fill, stroke, node);
		break;

	case K32("polygon"):
		AddSvgPolygon(state, fill, stroke, node);
		break;

	case K32("line"):
		AddSvgLine(state, fill, stroke, node);
		break;

	case K32("polyline"):
		AddSvgPolyLine(state, fill, stroke, node);
		break;
		
	case K32("svg"):
	case K32("g"):
	case K32("symbol"):
		goto Recurse;	//nothing to draw

	case K32("clipPath"):
	case K32("mask"):
	default:
		//not supported;
		goto Restore;
	}
	
	if (style.fill_colour.a)
	{
		ApplyTransform(matrix, fill);

		AddPointsWithColour(state.output, fill, style.fill_colour);
	}

	if (style.stroke_colour.a)
	{
		ApplyTransform(matrix, stroke);

		AddPointsWithColour(state.output, stroke, style.stroke_colour);
	}


	//recurse
	Recurse:
	for (auto & child : Data::GetXmlNodes(node))
	{
		AppendSvgNode(state, child);
	}


	//pop
	Restore:
	state.matrix = transform_z;
	state.style = style_z;
}

void ParseViewBoxTransform(const SVG & svg, Size render_size, SvgMatrix& matrix)
{
	SetIdentity(matrix.values);

	Float vx = svg.viewport.origin.x;
	Float vy = svg.viewport.origin.y;
	Float source_w = svg.viewport.size.w;
	Float source_h = svg.viewport.size.h;
	bool has_viewbox = source_w > 0.0f && source_h > 0.0f;

	if (source_w <= 0.0f || source_h <= 0.0f)
	{
		source_w = (render_size.w > 0.0f) ? render_size.w : 24.0f;
		source_h = (render_size.h > 0.0f) ? render_size.h : 24.0f;
	}

	Float target_w = render_size.w;
	Float target_h = render_size.h;

	Float sx = target_w / source_w;
	Float sy = target_h / source_h;
	Float align_tx = 0.0f;
	Float align_ty = 0.0f;

	if (has_viewbox)
	{
		auto [align_x, align_y] = svg.orientation;
		auto fit_mode = svg.fit;

		if (fit_mode != kFitModeStretch)
		{
			const Float scale = (fit_mode == kFitModeCover) ? Max(sx, sy) : Min(sx, sy);
			const Float fitted_w = source_w * scale;
			const Float fitted_h = source_h * scale;

			sx = scale;
			sy = scale;

			align_tx = Align1D(target_w, fitted_w, align_x);
			align_ty = Align1D(target_h, fitted_h, align_y);
		}
	}

	//1. scale to target
	Float S[6];
	MakeScale(S, sx, sy);
	MulMatrix(matrix.values, matrix.values, S);

	//2. viewBox translate
	if (has_viewbox)
	{
		Float T[6];
		MakeTranslate(T, -vx, -vy);
		MulMatrix(matrix.values, matrix.values, T);
	}

	if (align_tx != 0.0f || align_ty != 0.0f)
	{
		Float T[6];
		MakeTranslate(T, align_tx, align_ty);
		MulMatrix(matrix.values, T, matrix.values);
	}
}

REFLEX_END_INTERNAL

Reflex::Array < Reflex::GLX::Detail::SVG > Reflex::GLX::Detail::InspectSVG(const Data::PropertySet & xml)
{
	Array <SVG> output;

	if (Data::GetXmlTag(xml) == kSVG)
	{
		for (auto & child : Data::GetXmlNodes(xml))
		{
			if (MakeKey32(Data::GetXmlTag(child)) == K32("symbol"))
			{
				output.Push(InspectSvgNode(child));
			}
		}

		if (output)
		{
			return output;
		}
		else
		{
			output.Push(InspectSvgNode(xml));
		}
	}

	return output;
}

void Reflex::GLX::Detail::DecodeSVG(ColourPoints & output, const SVG & svg, Size render_size, Float32 curve_step)
{
	SvgState state = { .output = output };

	state.output.Allocate(64);
	state.fill_workspace.Allocate(64);
	state.stroke_workspace.Allocate(64);
	state.render_size = render_size;

	state.corner_step = curve_step;

	ParseViewBoxTransform(svg, render_size, state.matrix);
	
	AppendSvgNode(state, *svg.node);
}

const Reflex::File::ResourcePool::Ctr Reflex::GLX::Detail::kDecodeSVG = [](const File::ResourcePool::StreamContext & ctx, System::FileHandle & stream)
{
	auto xml = New<Data::PropertySet>();

	Data::kReflexXmlFormat->Decode(xml, File::ReadBytes(stream));

	return Cast<Reflex::Object>(xml);
};

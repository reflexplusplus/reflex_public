#pragma once

#include "../detail/functions/geometry.h"
#include "geometry.h"
#include "colour.h"




//
//Primary API

namespace Reflex::GLX
{

	using Points = Array <Point>;

	using ColourPoints = Array <ColourPoint>;

	struct Corners
	{
		Size tl, tr, bl, br;
	};

	enum PathJoin : UInt8
	{
		kPathJoinMiter,
		kPathJoinRound,
		kPathJoinBevel,

		kNumLineJoin
	};

	enum PathCap : UInt8
	{
		kPathCapButt,
		kPathCapRound,
		kPathCapSquare
	};

	enum FillRule : UInt8
	{
		kFillRuleNonZero,
		kFillRuleEvenOdd
	};


	void AddRectFill(Points & points, const Rect & rect);

	void AddRectFill(ColourPoints & colour_points, const Colour & colour, const Rect & rect);


	void AddRectOutline(Points & points, const Rect & rect, const Margin & width);

	void AddRectOutline(Points & points, const Rect & rect, const Margin & width, Size pixel_size);

	void AddRectOutline(ColourPoints & colour_points, const Colour & colour, const Rect & rect, const Margin & width);

	void AddRectOutline(ColourPoints & colour_points, const Colour & colour, const Rect & rect, const Margin & width, Size pixel_size);


	void AddRoundedFill(Points & points, const Rect & rect, const Corners & corners, Float32 corner_step = Detail::kCornerStepRadians);

	void AddRoundedFill(Points & points, const Rect & rect, Float corner, Float32 corner_step = Detail::kCornerStepRadians);


	void AddRoundedOutline(Points & points, const Rect & rect, const Margin & width, const Corners & corners, Float32 corner_step = Detail::kCornerStepRadians);

	void AddRoundedOutline(Points & points, const Rect & rect, const Margin & width, Float corner, Float32 corner_step = Detail::kCornerStepRadians);


	void AddEllipseFill(Points & points, const Rect & rect, Float start = 0.0f, Float sweep = k2Pif);

	void AddEllipseOutline(Points & points, const Rect & rect, Size width, Float start = 0.0f, Float sweep = k2Pif);

	void AddEllipseOutlineRoundCapped(Points & points, const Rect & rect, Size width, Float start, Float sweep);


	void AddTriangleFill(Points & points, const Rect & rect, Alignment direction);

	void AddTriangleOutline(Points & points, const Rect & rect, Float width, Alignment direction, Size pixel_size = kNormal);


	void AddRoundedTriangleFill(Points & points, const Rect & rect, Float corner, Alignment direction, Size pixel_size = kNormal);

	void AddRoundedTriangleOutline(Points & points, const Rect & rect, Float width, Float corner, Alignment direction, Size pixel_size = kNormal);


	void AddPolygonFill(Points & output, const Points::View & input);	//performs tesselsation

	void AddPolygonFill(Points & output, const ArrayView <Points::View> & input, FillRule fill_rule);	//compound polygon fill


	void AddDottedLine(Points & points, Point from, Point to, Size pixel_size);


	void AddPath(Points & points, const Points::View & path, bool closed, Float width, Size pixel_size = kNormal);

	void AddPath(Points & points, const Points::View & path, bool closed, Float width, PathJoin join, PathCap cap, Float miter_limit = 4.0f, Size pixel_size = kNormal);


	[[deprecated]] void AddGradientFill(ColourPoints & colour_points, const Colour & from, const Colour & to, const Rect & rect, bool y);


	void AddPointsWithColour(ColourPoints & colour_points, const Points::View & input, const Colour & colour);



	//transform

	void Rescale(const Points::Region & points, Scale scale);

	void Translate(const Points::Region & points, Point offset);

	void Rotate(const Points::Region & points, Point origin, Float angle_radians);


	void Rescale(const ColourPoints::Region & colour_points, Scale scale);

	void Translate(const ColourPoints::Region & colour_points, Point offset);

	void Rotate(const ColourPoints::Region & colour_points, Point origin, Float angle_radians);

	void ModulateColour(const ColourPoints::Region & colour_points, const Colour & colour);



	//to vbo

	TRef <System::Renderer::Graphic> CreateGraphic(const Points::View & points, System::Renderer::PrimitiveType primitive_type = System::Renderer::kPrimitiveTypeTriangles);

	TRef <System::Renderer::Graphic> CreateGraphic(const ColourPoints::View & colour_points, System::Renderer::PrimitiveType primitive_type = System::Renderer::kPrimitiveTypeTriangles);

}




//
//Reflex:: overloads

namespace Reflex
{

	inline System::ColourPoint LinearInterpolate(Float x, const System::ColourPoint & a, const System::ColourPoint & b)
	{
		return { LinearInterpolate(x, a.a, b.a), LinearInterpolate(x, a.b, b.b) };
	}

}




//
//macros

#define GLX_GET_POINT_WORKSPACE(NAME) auto & NAME = Reflex::GLX::Detail::g_point_workspace[(++Reflex::GLX::Detail::g_point_workspace_counter & 3)]; NAME.Clear();

#define GLX_GET_COLOURPOINT_WORKSPACE(NAME) auto & NAME = Reflex::GLX::Detail::g_colourpoint_workspace[(++Reflex::GLX::Detail::g_colourpoint_workspace_counter & 3)]; NAME.Clear();




//
//impl

REFLEX_NS(Reflex::GLX::Detail)

constexpr Corners MakeCorners(Size radius)
{
	return { radius, radius, radius, radius};
}

constexpr Corners MakeCorners(Float radius)
{
	return MakeCorners(Reflex::MakeSize(radius));
}


struct PathRescaler
{
	PathRescaler(Points & out, const Points::View & in, Size pixel_size);
	~PathRescaler();

	Points & output;
	Points path;
	const UInt start;
	const Scale scale;
};

void AddPathImpl(Points & buffer, const Points::View & path, bool closed, Float width);

void AddPathExImpl(Points & output, const Points::View & path, bool closed, Float width, PathJoin join, PathCap cap, Float miter_limit);


extern UInt g_point_workspace_counter;

extern UInt g_colourpoint_workspace_counter;

extern const ArrayRegion <Points> g_point_workspace;

extern const ArrayRegion <ColourPoints> g_colourpoint_workspace;

REFLEX_END

REFLEX_INLINE void Reflex::GLX::AddRectOutline(Points & points, const Rect & rect, const Margin & width, Size pixel_size)
{
	AddRectOutline(points, rect, { width.near * pixel_size, width.far * pixel_size });
}

REFLEX_INLINE void Reflex::GLX::AddRectOutline(ColourPoints & colourpoints, const Colour & colour, const Rect & rect, const Margin & width, Size pixel_size)
{
	AddRectOutline(colourpoints, colour, rect, { width.near * pixel_size, width.far * pixel_size });
}

REFLEX_INLINE void Reflex::GLX::AddRoundedFill(Points & points, const Rect & rect, Float corner, Float32 corner_step)
{
	auto min = rect.size * 0.5f;

	AddRoundedFill(points, rect, Detail::MakeCorners(Min(corner, Min(min.w, min.h))), corner_step);
}

REFLEX_INLINE void Reflex::GLX::AddRoundedOutline(Points & points, const Rect & rect, const Margin & width_in, Float corner, Float32 corner_step)
{
	auto min = rect.size * 0.5f;

	Margin width = { Min(width_in.near, min), Min(width_in.far, min) };

	AddRoundedOutline(points, rect, width, Detail::MakeCorners(Min(corner, Min(min.w, min.h))), corner_step);
}

REFLEX_INLINE void Reflex::GLX::AddPath(Points & output, const Points::View & path, bool closed, Float width, Size pixel_size)
{
	if (pixel_size == kNormal)
	{
		Detail::AddPathImpl(output, path, closed, width);
	}
	else
	{
		Detail::PathRescaler rescaler(output, path, pixel_size);

		Detail::AddPathImpl(output, rescaler.path, closed, width);
	}
}

REFLEX_INLINE void Reflex::GLX::AddPath(Points & output, const Points::View & path, bool closed, Float width, PathJoin join, PathCap cap, Float miter_limit, Size pixel_size)
{
	if (pixel_size == kNormal)
	{
		Detail::AddPathExImpl(output, path, closed, width, join, cap, miter_limit);
	}
	else
	{
		Detail::PathRescaler rescaler(output, path, pixel_size);

		Detail::AddPathExImpl(output, rescaler.path, closed, width, join, cap, miter_limit);
	}
}

REFLEX_INLINE Reflex::TRef <Reflex::System::Renderer::Graphic> Reflex::GLX::CreateGraphic(const Points::View & points, System::Renderer::PrimitiveType type)
{
	return Core::g_renderer->CreatePrimitives(type, points);
}

REFLEX_INLINE Reflex::TRef <Reflex::System::Renderer::Graphic> Reflex::GLX::CreateGraphic(const ColourPoints::View & colourpoints, System::Renderer::PrimitiveType type)
{
	return Core::g_renderer->CreatePrimitives(type, colourpoints);
}

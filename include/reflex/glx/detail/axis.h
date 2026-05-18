#pragma once

#include "[require].h"




//
// Detail

REFLEX_NS(Reflex::GLX::Detail)

class YAxis;

class XAxis;


Point MakePoint(bool yaxis, Float axis, Float ortho = 0.0f);

Size MakeSize(bool yaxis, Float axis, Float ortho = 0.0f);


void SetPoint(bool yaxis, Point & point, Float value);

Float GetPoint(bool yaxis, Point point);


void SetSize(bool yaxis, Size & size, Float value);

Float GetSize(bool yaxis, Size point);


void IncPoint(bool yaxis, Point & size, Float value);

void IncSize(bool yaxis, Size & size, Float value);


Pair <Float> GetPointComponents(bool yaxis, Point p);

Pair <Float> GetSizeComponents(bool yaxis, Size p);

REFLEX_END




//
// Detail::XAxis

class Reflex::GLX::Detail::XAxis
{
public:

	using Ortho = YAxis;


	static constexpr bool kX = true;

	static constexpr bool kY = false;


	static Point MakePoint(Float value, Float ortho = 0.0f);

	static void SetPoint(Point & point, Float value);

	static void SetPoint(Point & point, Float value, Float ortho);

	static void IncPoint(Point & point, Float delta);

	static Float GetPoint(Point point);


	static Size MakeSize(Float value, Float ortho = 0.0f);

	static void SetSize(Size & size, Float value);

	static void SetSize(Size & size, Float value, Float ortho);

	static void IncSize(Size & size, Float delta);

	static void IncSize(Size & size, Float delta, Float ortho);

	static Float GetSize(Size size);

};




//
// Detail::YAxis

class Reflex::GLX::Detail::YAxis
{
public:

	using Ortho = XAxis;

	
	static constexpr bool kX = false;

	static constexpr bool kY = true;


	static Point MakePoint(Float value, Float ortho = 0.0f);

	static void SetPoint(Point & point, Float value);

	static void SetPoint(Point & point, Float value, Float ortho);

	static void IncPoint(Point & point, Float delta);

	static Float GetPoint(Point point);


	static Size MakeSize(Float value, Float ortho = 0.0f);

	static void SetSize(Size & size, Float value);

	static void SetSize(Size & size, Float value, Float ortho);

	static void IncSize(Size & size, Float delta);

	static void IncSize(Size & size, Float delta, Float ortho);

	static Float GetSize(Size size);

};




//
//impl

REFLEX_INLINE Reflex::GLX::Point Reflex::GLX::Detail::XAxis::MakePoint(Float value, Float ortho)
{
	return { value, ortho };
}

REFLEX_INLINE void Reflex::GLX::Detail::XAxis::SetPoint(Point & point, Float value)
{
	point.x = value;
}

REFLEX_INLINE void Reflex::GLX::Detail::XAxis::SetPoint(Point & point, Float value, Float ortho)
{
	point = { value, ortho };
}

REFLEX_INLINE void Reflex::GLX::Detail::XAxis::IncPoint(Point & point, Float delta)
{
	point.x += delta;
}

REFLEX_INLINE Reflex::Float Reflex::GLX::Detail::XAxis::GetPoint(Point point)
{
	return point.x;
}

REFLEX_INLINE Reflex::GLX::Size Reflex::GLX::Detail::XAxis::MakeSize(Float value, Float ortho)
{
	return { value, ortho };
}

REFLEX_INLINE void Reflex::GLX::Detail::XAxis::SetSize(Size & size, Float value)
{
	size.w = value;
}

REFLEX_INLINE void Reflex::GLX::Detail::XAxis::SetSize(Size & size, Float value, Float ortho)
{
	size.w = value;

	size.h = ortho;
}

REFLEX_INLINE void Reflex::GLX::Detail::XAxis::IncSize(Size & size, Float delta)
{
	size.w += delta;
}

REFLEX_INLINE void Reflex::GLX::Detail::XAxis::IncSize(Size & size, Float delta, Float ortho)
{
	size.w += delta;

	size.h += ortho;
}

REFLEX_INLINE Reflex::Float Reflex::GLX::Detail::XAxis::GetSize(Size size)
{
	return size.w;
}

REFLEX_INLINE Reflex::GLX::Point Reflex::GLX::Detail::YAxis::MakePoint(Float value, Float ortho)
{
	return { ortho, value };
}

REFLEX_INLINE void Reflex::GLX::Detail::YAxis::SetPoint(Point & point, Float value)
{
	point.y = value;
}

REFLEX_INLINE void Reflex::GLX::Detail::YAxis::SetPoint(Point & point, Float value, Float ortho)
{
	point = { ortho, value };
}

REFLEX_INLINE Reflex::Float Reflex::GLX::Detail::YAxis::GetPoint(Point point)
{
	return point.y;
}

REFLEX_INLINE void Reflex::GLX::Detail::YAxis::IncPoint(Point & point, Float delta)
{
	point.y += delta;
}

REFLEX_INLINE Reflex::GLX::Size Reflex::GLX::Detail::YAxis::MakeSize(Float value, Float ortho)
{
	return { ortho, value };
}

REFLEX_INLINE void Reflex::GLX::Detail::YAxis::SetSize(Size & size, Float value)
{
	size.h = value;
}

REFLEX_INLINE void Reflex::GLX::Detail::YAxis::SetSize(Size & size, Float value, Float ortho)
{
	size.h = value;

	size.w = ortho;
}

REFLEX_INLINE void Reflex::GLX::Detail::YAxis::IncSize(Size & size, Float delta)
{
	size.h += delta;
}

REFLEX_INLINE void Reflex::GLX::Detail::YAxis::IncSize(Size & size, Float delta, Float ortho)
{
	size.h += delta;

	size.w += ortho;
}

REFLEX_INLINE Reflex::Float Reflex::GLX::Detail::YAxis::GetSize(Size size)
{
	return size.h;
}

REFLEX_INLINE Reflex::GLX::Point Reflex::GLX::Detail::MakePoint(bool yaxis, Float axis, Float ortho)
{
	Point rtn = { ortho, ortho };

	(&rtn.x)[yaxis] = axis;

	return rtn;
}

REFLEX_INLINE Reflex::GLX::Size Reflex::GLX::Detail::MakeSize(bool yaxis, Float axis, Float ortho)
{
	Size rtn = { ortho, ortho };

	(&rtn.w)[yaxis] = axis;

	return rtn;
}

REFLEX_INLINE void Reflex::GLX::Detail::SetPoint(bool yaxis, Point & point, Float value)
{
	(yaxis ? point.y : point.x) = value;
}

REFLEX_INLINE Reflex::Float Reflex::GLX::Detail::GetPoint(bool yaxis, Point point)
{
	return (&point.x)[yaxis];
}

REFLEX_INLINE void Reflex::GLX::Detail::SetSize(bool yaxis, Size & size, Float value)
{
	(yaxis ? size.h : size.w) = value;
}

REFLEX_INLINE void Reflex::GLX::Detail::IncPoint(bool yaxis, Point & point, Float value)
{
	(&point.x)[yaxis] += value;
}

REFLEX_INLINE void Reflex::GLX::Detail::IncSize(bool yaxis, Size & size, Float value)
{
	(&size.w)[yaxis] += value;
}

REFLEX_INLINE Reflex::Float Reflex::GLX::Detail::GetSize(bool yaxis, Size size)
{
	return (&size.w)[yaxis];
}

REFLEX_INLINE Reflex::Pair <Reflex::Float> Reflex::GLX::Detail::GetPointComponents(bool yaxis, Point p)
{
	return yaxis ? MakeTuple(p.y, p.x) : MakeTuple(p.x, p.y);
}

REFLEX_INLINE Reflex::Pair <Reflex::Float> Reflex::GLX::Detail::GetSizeComponents(bool yaxis, Size p)
{
	return yaxis ? MakeTuple(p.h, p.w) : MakeTuple(p.w, p.h);
}
#pragma once

#include "../../../include/reflex/glx/layout.h"
#include "../../../include/reflex/glx/detail/axis.h"
#include "../../../include/reflex/glx/detail/functions.h"
#include "../../../include/reflex/glx/functions/geometry.h"




//
//functions

REFLEX_NS(Reflex::GLX::Detail)

template <bool SNAP> REFLEX_INLINE Float RoundNearest(Float value)
{
	return SNAP ? Reflex::RoundNearest(value) : value;
}

template <bool SNAP> REFLEX_INLINE Float RoundDown(Float value)
{
	return SNAP ? Reflex::RoundDown(value + Core::kRoundingTolerance) : value;
}

template <bool SNAP> REFLEX_INLINE Float RoundUp(Float value)
{
	return SNAP ? Reflex::RoundUp(value) : value;
}

template <Orientation X, Orientation Y> REFLEX_INLINE void FloatItem(const Rect & inner, Object & item);

template <Orientation X, Orientation Y> REFLEX_INLINE Rect CalculateFloat(const Rect & inner, const Margin & margin, Size content_size, Size max, Scale scale);

template <class AXIS> REFLEX_INLINE Float Near(const Rect & rect, const Margin & margin, Size size)
{
	return AXIS::GetPoint(rect.origin) + AXIS::GetSize(margin.near);
}

template <class AXIS> REFLEX_INLINE Float Far(const Rect & rect, const Margin & margin, Size size)
{
	return AXIS::GetPoint(rect.origin) + AXIS::GetSize(rect.size) - (AXIS::GetSize(size) + AXIS::GetSize(margin.far));
}

template <class AXIS> REFLEX_INLINE Float Center(const Rect & rect, const Margin & margin, Size size)
{
	auto & [near,far] = margin;

	Float point = AXIS::GetPoint(rect.origin);

	point += RoundDown<true>((AXIS::GetSize(rect.size) - AXIS::GetSize(size)) * 0.5f);

	point += AXIS::GetSize(near);

	point -= AXIS::GetSize(far);

	return point;
}

REFLEX_END

template <Reflex::GLX::Orientation X, Reflex::GLX::Orientation Y> REFLEX_INLINE void Reflex::GLX::Detail::FloatItem(const Rect & inner, Object & item)
{
	item.ComputeLayout();

	auto & contentsize = item.contentsize;

	auto cstyle = item.GetComputedStyle();

	auto & margin = cstyle->GetMargin();

	auto max = cstyle->GetMinMax().b;

	auto scale = Reflex::MakeSize(cstyle->GetScale());

	item.SetRect(CalculateFloat<X,Y>(inner, margin, contentsize, max, scale));
}

template <Reflex::GLX::Orientation X_ORIENTATION, Reflex::GLX::Orientation Y_ORIENTATION> REFLEX_INLINE Reflex::GLX::Rect Reflex::GLX::Detail::CalculateFloat(const Rect & inner, const Margin & margin, Size content_size, Size max, Scale scale)
{
	if constexpr ((X_ORIENTATION == kOrientationFit) & (Y_ORIENTATION == kOrientationFit))
	{
		Size size = inner.size - (margin.near + margin.far);
		size = Min(size, max);
		size /= scale;

		Point position = { Center<XAxis>(inner, margin, size), Center<YAxis>(inner, margin, size) };

		return { position, size };
	}
	else
	{
		Size size = content_size / scale;

		Point position;

		switch (X_ORIENTATION)
		{
		case kOrientationNear:
			position.x = Near<XAxis>(inner, margin, content_size);
			break;

		case kOrientationCenter:
			position.x = Center<XAxis>(inner, margin, content_size);
			break;

		case kOrientationFar:
			position.x = Far<XAxis>(inner, margin, content_size);
			break;

		case kOrientationFit:
			size.w = inner.size.w - (margin.near.w + margin.far.w);
			size.w = Min(size.w, max.w);
			size.w /= scale.w;
			position.x = Center<XAxis>(inner, margin, size);
			break;
		};

		switch (Y_ORIENTATION)
		{
		case kOrientationNear:
			position.y = Near<YAxis>(inner, margin, content_size);
			break;

		case kOrientationCenter:
			position.y = Center<YAxis>(inner, margin, content_size);
			break;

		case kOrientationFar:
			position.y = Far<YAxis>(inner, margin, content_size);
			break;

		case kOrientationFit:
			size.h = inner.size.h - (margin.near.h + margin.far.h);
			size.h = Min(size.h, max.h);
			size.h /= scale.h;
			position.y = Center<YAxis>(inner, margin, size);
			break;
		};

		return { position, size };
	}
}

#pragma once

#include "../[require].h"
#include "../../object.h"
#include "../font.h"
#include "geometry.h"




//
//declarations

REFLEX_NS(Reflex::GLX::Detail)

//align

Float Align1D(Float owner_size, Float item_size, Orientation orientation);

Point Align(Size owner, Size size, Alignment alignment);

Point Align(Size owner, const Rect & rect, Alignment alignment);



//margin

template <class AXIS> Float AxisSum(const Margin & margin);



//text

Point AlignText(const Font & font, Float lineh, const WString::View & string, Size size, Alignment alignment);

bool TruncateRight(const Font & font, Float width, WString & string);

bool TruncateLeft(const Font & font, Float width, WString & string);

bool TruncatePath(const Font & font, Float width, WString & path);



//
//style properties

bool GetBool(const Data::PropertySet & properties, Key32 id, bool fallback = false);


Point ToPoint(const ArrayView <Float32> & floats, Point fallback = {});

Size ToSize(const ArrayView <Float32> & floats, Size fallback = {});

Margin ToMargin(const ArrayView <Float32> & floats, UInt16 stylesheet_flags = 0);

Colour ToColour(const ArrayView <Float32> & floats, const Colour & fallback = kWhite);

Colour ToColour(const CString::View & hex, const Colour & fallback = kWhite);

Float32 GetNumber(const Data::PropertySet & properties, Key32 id, Float32 fallback = 0.0f);



//layer properties

UInt8 ParseEnum(UInt32 value, const ArrayView <UInt32> & values, UInt8 default_value = 0);

inline UInt8 ParseEnum(Key32 value, const ArrayView <Key32> & values, UInt8 default_value = 0) { return ParseEnum(value.value, Reinterpret<ArrayView<UInt32>>(values), default_value); }

template <class ENUM, UInt SIZE> inline ENUM ParseEnum(Key32 value, const Key32(&values)[SIZE], ENUM default_value = ENUM(0)) { return ENUM(ParseEnum(value, ArrayView<Key32>(values, SIZE), default_value)); }


Alignment ParseAlignment(Key32 value, Alignment default_value = kAlignmentTopLeft);

Orientation ParseOrientation(Key32 value, Orientation default_value = kOrientationNear);



//geometry and math

Point ToFloat(const System::iPoint & ipoint);

Size ToFloat(const System::iSize & isize);

Rect ToFloat(const System::iRect & irect);


System::iRect ToInt(const Rect & rect);


Float SnapToPixels(Float value);

Point SnapToPixels(Point point);

Size SnapToPixels(Size size);

Rect SnapToPixels(const Rect & rect);


Point Snap(const Point & point, const Size & pix);

Size Snap(const Size & size, const Size & pix);

Rect Snap(const Rect & rect, const Size & pix);


Rect ClipRect(const Rect & clip, const Rect & target);

Rect ConstrainRect(const Rect & bounds, const Rect & rect);



//events

void PerformStandardScroll(Object & viewbar, Float vo, Float delta, bool y, bool inverted, bool fast);



//transasction

bool BeginTransaction(Object & object, UInt idx);

void PerformTransaction(Object & object, UInt idx, Float32 value = 0.0f);

void EndTransaction(Object & object, UInt idx, bool cancel = false);



//layout

Size ComputeContentSize(Object & object);

REFLEX_END




//
//impl

REFLEX_NS(Reflex::GLX::Detail)

extern const Scale kAlignOrigin[9];

extern const Scale kAlignInvert[9];

extern const Pair <Float> kOrientationToAlign1D[4];

extern const Key32 kAlignmentKeys[kNumAlignment];

REFLEX_INLINE Float Align1D(Float owner_size, Float item_size, Orientation orientation, const Pair <Float> * OrientationToAlign1D)
{
	auto mults = OrientationToAlign1D[orientation];

	return (owner_size * mults.a) - (item_size * mults.b);
}

REFLEX_END

REFLEX_INLINE Reflex::Float Reflex::GLX::Detail::Align1D(Float owner_size, Float item_size, Orientation orientation)
{
	return Align1D(owner_size, item_size, orientation, kOrientationToAlign1D);
}

REFLEX_INLINE Reflex::GLX::Point Reflex::GLX::Detail::ToPoint(const ArrayView <Float32> & floats, Point fallback)
{
	if (floats)
	{
		return { floats.GetFirst(), floats.GetLast() };
	}
	else
	{
		return fallback;
	}
}

REFLEX_INLINE Reflex::GLX::Size Reflex::GLX::Detail::ToSize(const ArrayView <Float32> & floats, Size fallback)
{
	return Reinterpret<Size>(ToPoint(floats, Reinterpret<Point>(fallback)));
}

REFLEX_INLINE Reflex::GLX::Margin Reflex::GLX::Detail::ToMargin(const ArrayView <Float32> & floats, UInt16 stylesheet_flags)
{
	UInt shift = stylesheet_flags & 1;	//margin_syntax==css, only flag for now

	switch (floats.size)
	{
	case 1:
		return MakeMargin(floats.GetFirst());

	case 2:
		return MakeMargin(floats[shift], floats[(1 + shift) & 1]);

	case 4:
		shift *= 3;
		return MakeMargin(floats[shift], floats[(1 + shift) & 3], floats[(2 + shift) & 3], floats[(3 + shift) & 3]);
	}

	return {};
}

REFLEX_INLINE Reflex::Float32 Reflex::GLX::Detail::GetNumber(const Data::PropertySet & properties, Key32 id, Float32 fallback)
{
	if (auto value = Data::GetFloat32Array(properties, id)) return value.GetFirst();

	return fallback;
}

REFLEX_INLINE Reflex::GLX::Alignment Reflex::GLX::Detail::ParseAlignment(Key32 string, Alignment default_value)
{
	return Alignment(ParseEnum(string, ToView(kAlignmentKeys), default_value));
}

REFLEX_INLINE Reflex::GLX::Orientation Reflex::GLX::Detail::ParseOrientation(Key32 string, Orientation default_value)
{
	return Orientation(ParseEnum(string, ToView(kOrientationKeys), default_value));
}

template <class AXIS> REFLEX_INLINE Reflex::Float Reflex::GLX::Detail::AxisSum(const Margin & margin)
{
	return AXIS::GetSize(margin.near) + AXIS::GetSize(margin.far);
}

REFLEX_INLINE Reflex::GLX::Point Reflex::GLX::Detail::ToFloat(const System::iPoint & ipoint)
{
	return { Float(ipoint.x), Float(ipoint.y) };
}

REFLEX_INLINE Reflex::GLX::Size Reflex::GLX::Detail::ToFloat(const System::iSize & isize)
{
	return { Float(isize.w), Float(isize.h) };
}

REFLEX_INLINE Reflex::GLX::Rect Reflex::GLX::Detail::ToFloat(const System::iRect & irect)
{
	return { ToFloat(irect.origin), ToFloat(irect.size) };
}

REFLEX_INLINE Reflex::Float Reflex::GLX::Detail::SnapToPixels(Float value)
{
	return Quantise(value, kPixelSize);
}

REFLEX_INLINE Reflex::GLX::Point Reflex::GLX::Detail::SnapToPixels(Point point)
{
	return { SnapToPixels(point.x), SnapToPixels(point.y) };
}

REFLEX_INLINE Reflex::GLX::Size Reflex::GLX::Detail::SnapToPixels(Size size)
{
	return { SnapToPixels(size.w), SnapToPixels(size.h) };
}

REFLEX_INLINE Reflex::GLX::Rect Reflex::GLX::Detail::SnapToPixels(const Rect & rect)
{
	Float x = SnapToPixels(rect.origin.x);
	Float y = SnapToPixels(rect.origin.y);
	Float w = SnapToPixels(rect.size.w - (x - rect.origin.x));
	Float h = SnapToPixels(rect.size.h - (y - rect.origin.y));

	return { { x, y }, { w, h } };
}

REFLEX_INLINE Reflex::GLX::Point Reflex::GLX::Detail::Snap(const Point & point, const Size & pix)
{
	return { Quantise(point.x, pix.w), Quantise(point.y, pix.h) };
}

REFLEX_INLINE Reflex::GLX::Size Reflex::GLX::Detail::Snap(const Size & size, const Size & pix)
{
	return { Quantise(size.w, pix.w), Quantise(size.h, pix.h) };
}

REFLEX_INLINE Reflex::GLX::Rect Reflex::GLX::Detail::Snap(const Rect & rect, const Size & pix)
{
	Float x = Quantise(rect.origin.x, pix.w);
	Float y = Quantise(rect.origin.y, pix.h);
	Float w = Quantise(rect.size.w - (x - rect.origin.x), pix.w);
	Float h = Quantise(rect.size.h - (y - rect.origin.y), pix.h);

	return { { x, y }, { w, h } };
}

REFLEX_INLINE Reflex::GLX::Rect Reflex::GLX::Detail::ClipRect(const Rect & clip, const Rect & target)
{
	auto min_pos = Max(clip.origin, target.origin);

	auto max_pos = Min(clip.origin + Reinterpret<Point>(clip.size), target.origin + Reinterpret<Point>(target.size));

	auto new_size = Reinterpret<Size>(max_pos - min_pos);

	return { min_pos, new_size };
}

REFLEX_INLINE Reflex::GLX::Rect Reflex::GLX::Detail::ConstrainRect(const Rect & bounds, const Rect & rect)
{
	auto rtn = rect;

	Float max_x = bounds.origin.x + bounds.size.w;
	
	Float max_y = bounds.origin.y + bounds.size.h;

	if (rtn.origin.x + rtn.size.w > max_x) rtn.origin.x = max_x - rtn.size.w;

	if (rtn.origin.x < bounds.origin.x) rtn.origin.x = bounds.origin.x;

	if (rtn.origin.y + rtn.size.h > max_y) rtn.origin.y = max_y - rtn.size.h;

	if (rtn.origin.y < bounds.origin.y) rtn.origin.y = bounds.origin.y;

	return rtn;
}

REFLEX_INLINE Reflex::GLX::Size Reflex::GLX::Detail::ComputeContentSize(Object & object)
{
	object.ComputeLayout();

	return object.contentsize;
}

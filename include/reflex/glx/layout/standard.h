#pragma once

#include "detail/standard_layout.h"
#include "../object.h"




//
//Primary API

namespace Reflex::GLX
{

	enum Orientation : UInt8
	{
		kOrientationNear,
		kOrientationCenter,
		kOrientationFar,
		kOrientationFit,

		kNumOrientation,
	};

	enum FlowFlags : UInt8
	{
		kFlowX = 0,
		kFlowY = MakeBit(Detail::kStandardLayoutY),
		kFlowInvert = MakeBit(Detail::kStandardLayoutInvert),
		kFlowCenter = MakeBit(Detail::kStandardLayoutCenter),
	};


	void SetFlow(Object & object, UInt8 flowflags);

	bool GetAxis(const Object & object);

	void EnableAutoFit(Object & object, bool x, bool y);


	template <class TYPE> auto AddInline(Object & parent, TYPE && child, Orientation ortho = kOrientationFit);

	template <class TYPE> auto AddInlineFlex(Object & parent, TYPE && child, Orientation ortho = kOrientationFit);


	template <class TYPE> auto AddFloat(Object & parent, TYPE && child, Orientation x, Orientation y);

	template <class TYPE> auto AddFloat(Object & parent, TYPE && child, Alignment alignment);


	template <class TYPE> auto AddStretch(Object & parent, TYPE && child);


	template <class TYPE> auto AddAbsolute(Object & parent, TYPE && child);

	template <class TYPE> auto AddAbsolute(Object & parent, TYPE && child, const Point & position);


	template <bool FLEX> void EnableInline(Object & object, Orientation ortho = kOrientationFit);

	void EnableFloat(Object & object, Alignment alignment);

	void EnableFloat(Object & object, Orientation x, Orientation y);

	void EnableAbsolute(Object & object);

}




//
//impl

REFLEX_NS(Reflex::GLX::Detail)

extern const Pair <Orientation> kAlignmentToOrientation[kNumAlignment];

void AddItem(Object & object, TRef <Object> child, Positioning positioning, Orientation axis, Orientation ortho);

void SetPositioning(Object & object, Positioning positioning, Orientation axis, Orientation ortho);

Tuple <Positioning,Orientation,Orientation> GetPositioning(const Object & object);

REFLEX_END

inline void Reflex::GLX::Detail::AddItem(Object & object, TRef <Object> child, Positioning positioning, Orientation axis, Orientation ortho)
{
	SetPositioning(child, positioning, axis, ortho);

	child->SetParent(object);
}

template <class TYPE> REFLEX_INLINE auto Reflex::GLX::AddInline(Object & parent, TYPE && child, Orientation ortho)
{
	auto & object = Deref(child);

	EnableInline<false>(object, ortho);

	object.SetParent(parent);

	return TRef<NonRefT<decltype(object)>>(object);
}

template <class TYPE> REFLEX_INLINE auto Reflex::GLX::AddInlineFlex(Object & parent, TYPE && child, Orientation ortho)
{
	auto & object = Deref(child);

	EnableInline<true>(object, ortho);

	object.SetParent(parent);

	return TRef<NonRefT<decltype(object)>>(object);
}

template <class TYPE> REFLEX_INLINE auto Reflex::GLX::AddFloat(Object & parent, TYPE && child, Orientation x, Orientation y)
{
	auto & object = Deref(child);

	EnableFloat(object, x, y);

	object.SetParent(parent);

	return TRef<NonRefT<decltype(object)>>(object);
}

template <class TYPE> REFLEX_INLINE auto Reflex::GLX::AddFloat(Object & parent, TYPE && child, Alignment alignment)
{
	auto & object = Deref(child);

	EnableFloat(object, alignment);

	object.SetParent(parent);

	return TRef<NonRefT<decltype(object)>>(object);
}

template <class TYPE> REFLEX_INLINE auto Reflex::GLX::AddStretch(Object & parent, TYPE && child)
{
	return AddFloat(parent, child, kOrientationFit, kOrientationFit);
}

template <class TYPE> REFLEX_INLINE auto Reflex::GLX::AddAbsolute(Object & parent, TYPE && child)
{
	auto & object = Deref(child);

	EnableAbsolute(object);

	object.SetParent(parent);

	return TRef<NonRefT<decltype(object)>>(object);
}

template <class TYPE> REFLEX_INLINE auto Reflex::GLX::AddAbsolute(Object & parent, TYPE && child, const Point & position)
{
	auto & object = Deref(child);

	object.GLX::Object::SetPosition(position);

	return AddAbsolute(parent, object);
}

REFLEX_INLINE bool Reflex::GLX::GetAxis(const Object & object)
{
	return BitCheck(object.GetLayoutFlags(), Detail::kStandardLayoutY);
}

REFLEX_INLINE void Reflex::GLX::SetFlow(Object & object, UInt8 flowflags)
{
	flowflags = (flowflags & 15) | (object.GetLayoutFlags() & Bits<false,false,false,false,true,true,true,true>::value);

	object.SetLayoutFlags(flowflags);
}

REFLEX_INLINE void Reflex::GLX::EnableAutoFit(Object & object, bool x, bool y)
{
	UInt8 flags = (MakeBits(x, y) << 4) | (object.GetLayoutFlags() & Bits<true,true,true,true,false,false,true,true>::value);

	object.SetLayoutFlags(flags);
}

REFLEX_INLINE void Reflex::GLX::Detail::SetPositioning(Object & object, Positioning positioning, Orientation axis, Orientation ortho)
{
	UInt8 flags = UInt8(positioning) | (UInt8(axis) << 2) | (UInt8(ortho) << 4)/* | (object->GetPositioningFlags() & 192)*/;

	object.SetPositioningFlags(flags);
}

REFLEX_INLINE Reflex::Tuple <Reflex::GLX::Detail::Positioning,Reflex::GLX::Orientation,Reflex::GLX::Orientation> Reflex::GLX::Detail::GetPositioning(const Object & object)
{
	auto flags = object.GetPositioningFlags();

	return { Positioning(flags & 3), Orientation((flags >> 2) & 3), Orientation((flags >> 4) & 3) };
}

template <bool FLEX> REFLEX_INLINE void Reflex::GLX::EnableInline(Object & object, Orientation ortho)
{
	Detail::SetPositioning(object, Detail::kPositioningInline, FLEX ? kOrientationFit : kOrientationNear, ortho);
}

REFLEX_INLINE void Reflex::GLX::EnableFloat(Object & object, Orientation x, Orientation y)
{
	Detail::SetPositioning(object, Detail::kPositioningFloat, x, y);
}

REFLEX_INLINE void Reflex::GLX::EnableFloat(Object & object, Alignment alignment)
{
	auto orientation = Detail::kAlignmentToOrientation[alignment];

	EnableFloat(object, orientation.a, orientation.b);
}

REFLEX_INLINE void Reflex::GLX::EnableAbsolute(Object & object)
{
	Detail::SetPositioning(object, Detail::kPositioningAbsolute, kOrientationNear, kOrientationNear);
}
#pragma once

#include "reflex/glx.h"




//
//Primary API

namespace Reflex::GLX
{

	//run

	void Run(Object & target, Key32 id, WillRetain <Animation> scene);

	void Run(Object & target, Key32 id, Float32 time, WillRetain <Animation> scene);

	void Run(Object & target, Key32 id, Float32 time, InterpolatedAnimation::Easing easing, WillRetain <InterpolatedAnimation> scene);

	void Stop(Object & object, Key32 id);



	//periodic

	[[nodiscard]] Unretained <Animation> CreateStateAnimation(Key32 state = kSelectedState);

	[[nodiscard]] Unretained <Animation> CreateCallbackAnimation(const Function <void(Object & target)> & callback);



	//linear

	[[nodiscard]] Unretained <InterpolatedAnimation> CreateWaitAnimation();

	[[nodiscard]] Unretained <InterpolatedAnimation> CreatePositionAnimation(bool y, Float from, Float to);

	[[nodiscard]] Unretained <InterpolatedAnimation> CreateOpacityAnimation(Key32 opacity_id, Float from, Float to, Detail::ComputedStyle::Render render = Detail::ComputedStyle::kRenderFalse);


	[[nodiscard]] Unretained <InterpolatedAnimation> CreateFloatPropertyAnimation(Key32 property_id, Float from, Float to);

	[[nodiscard]] Unretained <InterpolatedAnimation> CreatePointPropertyAnimation(Key32 property_id, Point from, Point to);

	[[nodiscard]] Unretained <InterpolatedAnimation> CreateSizePropertyAnimation(Key32 property_id, Size from, Size to);

	[[nodiscard]] Unretained <InterpolatedAnimation> CreateColourPropertyAnimation(Key32 property_id, const Colour & from, const Colour & to);

	[[nodiscard]] Unretained <InterpolatedAnimation> CreateMarginPropertyAnimation(Key32 property_id, const Margin & from, const Margin & to);


	[[nodiscard]] Unretained <InterpolatedAnimation> CreateInterpolatedAnimation(const Function <void(Object & target, Float x)> & callback);



	//logarithmic

	[[nodiscard]] Unretained <Animation> CreateMaxBoundsAnimation(Key32 bounds_id, bool yaxis, Float from, Float to);

	[[nodiscard]] Unretained <Animation> CreateLogarithmicAnimation(Float from, Float to, const Function <void(Object & target, Float x)> & callback, Float decay_factor = 0.005f);



	//container

	template <class TYPE> auto AddScene(ContainerAnimation & parent, TYPE && child);

	template <class TYPE> auto AddScene(ContainerAnimation & parent, Float32 time, TYPE && child);

	template <class TYPE> auto AddScene(ContainerAnimation & parent, Float32 time, InterpolatedAnimation::Easing easing, TYPE && child);

}




//
//impl

REFLEX_NS(Reflex::GLX::Detail)

[[nodiscard]] Unretained <InterpolatedAnimation> CreatePropertyAnimation(Address property_adr, const ArrayView <Float32> & from, const ArrayView <Float32> & to);

[[nodiscard, deprecated]] Unretained <Animation> CreateZoomAnimation(Key32 magnification_id, Float from, Float to);	//use PropertyAnimation and Scale layer

REFLEX_END

inline Reflex::Unretained <Reflex::GLX::InterpolatedAnimation> Reflex::GLX::CreateFloatPropertyAnimation(Key32 property_id, Float from, Float to)
{
	return Detail::CreatePropertyAnimation(MakeAddress<Data::Float32Property>(property_id), { &from, 1 }, { &to, 1 });
}

inline Reflex::Unretained <Reflex::GLX::InterpolatedAnimation> Reflex::GLX::CreatePointPropertyAnimation(Key32 property_id, Point from, Point to)
{
	return Detail::CreatePropertyAnimation(MakeAddress<PointProperty>(property_id), { &from.x, 2 }, { &to.x, 2 });
}

inline Reflex::Unretained <Reflex::GLX::InterpolatedAnimation> Reflex::GLX::CreateSizePropertyAnimation(Key32 property_id, Size from, Size to)
{
	return Detail::CreatePropertyAnimation(MakeAddress<SizeProperty>(property_id), { &from.w, 2 }, { &to.w, 2 });
}

inline Reflex::Unretained <Reflex::GLX::InterpolatedAnimation> Reflex::GLX::CreateColourPropertyAnimation(Key32 property_id, const Colour & from, const Colour & to)
{
	return Detail::CreatePropertyAnimation(MakeAddress<ColourProperty>(property_id), { &from.r, 4 }, { &to.r, 4 });
}

inline Reflex::Unretained <Reflex::GLX::InterpolatedAnimation> Reflex::GLX::CreateMarginPropertyAnimation(Key32 property_id, const Margin & from, const Margin & to)
{
	return Detail::CreatePropertyAnimation(MakeAddress<MarginProperty>(property_id), { &from.near.w, 4 }, { &to.near.w, 4 });
}

template <class TYPE> REFLEX_INLINE auto Reflex::GLX::AddScene(ContainerAnimation & parent, TYPE && scene)
{
	auto & ref = Deref(scene);

	parent.Add(ref);

	return AlreadyRetained<NonRefT<decltype(ref)>>(ref);
}

template <class TYPE> REFLEX_INLINE auto Reflex::GLX::AddScene(ContainerAnimation & parent, Float32 time, TYPE && scene)
{
	auto & ref = Deref(scene);

	ref.SetTime(time);

	parent.Add(ref);

	return AlreadyRetained<NonRefT<decltype(ref)>>(ref);
}

template <class TYPE> REFLEX_INLINE auto Reflex::GLX::AddScene(ContainerAnimation & parent, Float32 time, InterpolatedAnimation::Easing easing, TYPE && scene)
{
	auto & ref = Deref(scene);

	ref.SetTime(time);

	ref.SetEasing(easing);

	parent.Add(ref);

	return AlreadyRetained<NonRefT<decltype(ref)>>(ref);
}

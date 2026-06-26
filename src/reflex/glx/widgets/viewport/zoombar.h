#pragma once

#include "[require].h"




//
//zoombar

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

struct ZoomBar : public AbstractViewBar
{
	ZoomBar();


	void SetFlow(UInt8 flowflags) override;

	void OnSetRange() override;

	void OnSetStyle(const Style & style) override;

	bool OnEvent(Object & src, Event & e) override;


	static void OnAccommodate(ZoomBar & self, bool & isresponsive, Size & contentsize);

	template <class AXIS> static void OnAlign(ZoomBar & self, bool isresponsive, Float & contenth);


	void Increment(Point delta, UInt8 modifier_keys);


	using AbstractViewBar::EmitTransaction;


	Reference <RangeProperty> m_range, m_region;

	Float m_min_size = 16.0f;

	Float m_extent_z = 0.0f;

};

REFLEX_END_INTERNAL

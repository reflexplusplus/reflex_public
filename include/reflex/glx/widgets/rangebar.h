#pragma once

#include "viewport/abstract_viewbar.h"




//
//Primary API

namespace Reflex::GLX
{

	class RangeBar;

}




//
//RangeBar

class Reflex::GLX::RangeBar : public AbstractViewBar
{
public:

	REFLEX_OBJECT(GLX::RangeBar, AbstractViewBar);


	
	//events

	static constexpr auto kTransaction = GLX::kTransaction;



	//lifetime

	RangeBar();



	//setup

	void EnableResize(bool enable = true);



protected:

	virtual void OnSetMinSize(Float32 min_size) override;

	virtual void OnSetRange() override;

	virtual void OnSetStyle(const Style & style) override;

	virtual bool OnEvent(Object & src, Event & e) override;



private:

	struct Layout;


	bool m_enable_resize;

	bool m_transaction_active;


	Reference <Data::Float32Property> m_min_size_property;

	Reference <RangeProperty> m_range_property, m_region_property;


	Float m_pixsize;

	GLX::Object m_bar;

};

REFLEX_SET_TRAIT(Reflex::GLX::RangeBar, IsSingleThreadExclusive);

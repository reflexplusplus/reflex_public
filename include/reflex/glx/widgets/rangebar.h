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


	void SetSnap(Float snap);

	Float GetSnap() const;



protected:

	virtual void OnSetRange() override;

	virtual void OnSetStyle(const Style & style) override;

	virtual bool OnEvent(Object & src, Event & e) override;



private:

	struct Layout;


	bool m_enable_resize;

	UInt8 m_transactionflag;

	Float m_snap;

	Float m_pixsize;

	Float m_minsize;

	GLX::Object m_bar;


	Reference <RangeProperty> m_range, m_region;

};

REFLEX_SET_TRAIT(Reflex::GLX::RangeBar, IsSingleThreadExclusive);

#pragma once

#include "../../defines.h"
#include "../../layout.h"




//
//declarations

namespace Reflex::GLX
{

	struct AbstractViewBar;

}




//
//AbstractViewBar

struct Reflex::GLX::AbstractViewBar : public Object
{
public:

	REFLEX_OBJECT(GLX::AbstractViewBar, Object);

	static AbstractViewBar & null;

	REFLEX_GLX_EVENT_ID(Jump);		//@Float offset



	//setup
	
	void SetMinSize(Float min_size) { OnSetMinSize(min_size); }


	
	//access

	void SetRange(Float extent, Range region);

	Tuple <Float,Range> GetRange() const;



protected:

	using Object::Object;

	bool EmitTransaction(TransactionStage stage, Range region, Orientation edge = kOrientationCenter);

	bool EmitJump(Float region_start);


	virtual void OnSetMinSize(Float32 min_size) {}

	virtual void OnSetRange() {}



private:

	Float m_extent = 1.0f;

	Range m_region = kNormalRange;

};

REFLEX_SET_TRAIT(Reflex::GLX::AbstractViewBar, IsSingleThreadExclusive);




//
//impl

REFLEX_INLINE Reflex::Tuple <Reflex::Float, Reflex::GLX::Range> Reflex::GLX::AbstractViewBar::GetRange() const
{
	return { m_extent, m_region };
}

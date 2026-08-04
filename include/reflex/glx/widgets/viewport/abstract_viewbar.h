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
	
	virtual void SetFlow(UInt8 flowflags) { GLX::SetFlow(*this, flowflags); }


	
	//access

	void SetRange(Float extent, Range region);

	Tuple <Float,Range> GetRange() const;



	//info

	const Float extent = 1.0f;

	const Range region = kNormalRange;



protected:

	bool EmitTransaction(TransactionStage stage, Range region, Orientation edge = kOrientationCenter);

	bool EmitJump(Float region_start);


	virtual void OnSetRange() {}


	using Object::Object;

};

REFLEX_SET_TRAIT(Reflex::GLX::AbstractViewBar, IsSingleThreadExclusive);




//
//impl

REFLEX_INLINE Reflex::Tuple <Reflex::Float, Reflex::GLX::Range> Reflex::GLX::AbstractViewBar::GetRange() const
{
	return { extent, region };
}

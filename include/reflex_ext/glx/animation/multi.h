#pragma once

#include "reflex/glx.h"




//
//Primary API

namespace Reflex::GLX
{

	class Multi;

}




//
//Multi

class Reflex::GLX::Multi : public ContainerAnimation
{
public:

	REFLEX_OBJECT(Multi, ContainerAnimation);



	//lifetime

	Multi();



private:

	virtual void OnBegin() override;

	virtual bool OnClock(Float delta) override;

	virtual void OnSkip() override;

};

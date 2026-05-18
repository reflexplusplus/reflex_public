#pragma once

#include "reflex/glx.h"




//
//Primary API

namespace Reflex::GLX
{

	class PlayList;

}




//
//PlayList

class Reflex::GLX::PlayList : public ContainerAnimation
{
public:

	REFLEX_OBJECT(PlayList, ContainerAnimation);



	//lifetime

	PlayList();



	//setup

	void EnableLoop(bool enable = true);




private:

	virtual void OnClear() override;


	virtual void OnBegin() override;

	virtual bool OnClock(Float delta) override;

	virtual void OnSkip() override;


	Reference m_current;

	bool m_loop;

	bool m_play;

};

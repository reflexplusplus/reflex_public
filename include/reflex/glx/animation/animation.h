#pragma once

#include "../object.h"




//
//Secondary API (Primary: CreateXXXAnimation, Run)

namespace Reflex::GLX
{

	class Animation;

}




//
//Animation

class Reflex::GLX::Animation :
	public Reflex::Object,
	public Detail::Countable <MakeKey32("Animation")>
{
public:

	REFLEX_OBJECT(GLX::Animation, Object);

	static Animation & null;



	//setup

	void SetTarget(GLX::Object & object) { OnSetTarget(object); }

	void SetTime(Float32 time) { OnSetTime(time); }



	//access

	void Play();



protected:

	//lifetime

	Animation();



	//callbacks

	virtual void OnSetTime(Float32 time) {}

	virtual void OnSetTarget(GLX::Object & object) {}

	virtual void OnBegin() {}

	virtual bool OnClock(Float delta) = 0;		//return true to continue, false when complete

	virtual void OnSkip() = 0;



	//for containers

	void Reset();

	bool Process(Float delta);



private:

	friend class ContainerAnimation;


	bool Begin(Float delta);

	template <bool CALL> bool Skip(Float delta);


	decltype (&Animation::Begin) m_callback;

	static const decltype (&Animation::Begin) kOnProcess[2];

};

REFLEX_SET_TRAIT(Reflex::GLX::Animation, IsSingleThreadExclusive)




//
//impl

REFLEX_NS(Reflex::GLX::Detail)

const UInt32 & GetNumActiveAnimation();

REFLEX_END

REFLEX_INLINE bool Reflex::GLX::Animation::Process(Float delta)
{
	return (this->*m_callback)(delta);
}

#pragma once

#include "../defines.h"




//
//Secondary API (Primary: functions.h)

namespace Reflex::GLX
{

	class Event;

}




//
//Event

class Reflex::GLX::Event : public Reflex::Item <Event,false,Data::PropertySet>
{
public:

	REFLEX_OBJECT(GLX::Event, Data::PropertySet);

	static Event & null;



	//lifetime

	Event();

	Event(Key32 id);

	Event(Key32 id, Data::PropertySet && params);

	Event(const Event & e) = delete;

	Event(Event && e) = delete;



	//clone

	virtual TRef <Event> Clone() const;



	//id

	Key32 id;



private:

	friend class GLX::Object;

	struct Scope;


	static List st_list;

};

REFLEX_SET_TRAIT(Reflex::GLX::Event, IsSingleThreadExclusive);




//
//impl

inline Reflex::GLX::Event::Event::Event()
	: id(kNullKey)
{
}

inline Reflex::GLX::Event::Event(Key32 type)
	: id(type)
{
}

inline Reflex::TRef <Reflex::GLX::Event> Reflex::GLX::Event::Clone() const
{
	auto clone = New<Event>(id);

	Data::Assimilate(clone, *this);

	return clone;
}

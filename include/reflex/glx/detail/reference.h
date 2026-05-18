#pragma once

#include "[require].h"
#include "../object.h"




//
//

REFLEX_NS(Reflex::GLX::Detail)

template <bool RETAIN>
struct ObjectRefObject : public Reflex::Object
{
	ObjectRefObject(GLX::Object & object = GLX::Object::null)
		: ref(object)
	{
		if constexpr (RETAIN) Retain(object);
	}

	ObjectRefObject(const ObjectRefObject & value)
		: ref(value.ref)
	{
		if constexpr (RETAIN) Retain(*ref);
	}

	~ObjectRefObject()
	{
		if constexpr (RETAIN) Release(*ref);
	}

	GLX::Object & operator*(){return *ref;}

	GLX::Object * operator->(){return ref.Adr();}

	ObjectRefObject & operator=(GLX::Object & object)
	{
		REFLEX_STATIC_ASSERT(!RETAIN);

		ref = object;

		return *this;
	}

	ObjectRefObject & operator=(const ObjectRefObject & value)
	{
		REFLEX_STATIC_ASSERT(!RETAIN);

		ref = value.ref;

		return *this;
	}

	bool operator==(const ObjectRefObject & value) const
	{
		return ref.Adr() == value.ref.Adr();
	}

	Core::WeakReference ref;
};

using StrongRefObject = ObjectRefObject <true>;

REFLEX_END




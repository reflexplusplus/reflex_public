#pragma once

#include "reflex/glx/warnings.h"
#include "reflex/glx/object.h"




//
//declarations

REFLEX_NS(Reflex::GLX)

constexpr UInt32 kDelegates = K32("{delegates}");
constexpr UInt32 kStates = K32("{states}");
constexpr UInt32 kMods = K32("{mods}");

struct NullObject : public Object
{
	REFLEX_OBJECT(GLX::NullObject, Object);

	void Clear()
	{
		GLX::Object::Clear();

		Data::PropertySet::Clear();
	}

	virtual void OnSetProperty(Address address, Reflex::Object & object) override
	{
		#if REFLEX_DEBUG
		switch (address.id.value)
		{
		case kDelegates:
		case kStates:
		case kMods:
			break;

		default:
			REFLEX_DEBUG_WARN(output, GLX::null_object_set, false, "GLX::NullObject::OnSetProperty @", object.object_t->tname);
			break;
		}
		#endif

		Data::PropertySet::OnSetProperty(address, object);
	}

	virtual void OnSetStyle(const Style & style) override
	{
		REFLEX_DEBUG_WARN(output, GLX::null_object_set, false, "GLX::NullObject::OnSetStyle ", kSingleQuote, GetKey(style.id), kSingleQuote);
	}

	virtual bool OnEvent(Object & src, GLX::Event & e) override { /*critical for EmitEx implementation*/ return true; }

	virtual void OnReleaseData() override
	{
		REFLEX_ASSERT(!GetNumItem());

		Object::OnReleaseData();
	}
}; 

typedef ObjectOf < Sequence <Key32> > States;

REFLEX_END

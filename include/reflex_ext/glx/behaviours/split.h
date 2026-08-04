#pragma once

#include "reflex/glx.h"




//
//Addon API

namespace Reflex::GLX
{

	class SplitBehaviour;

}




//
//SplitBehaviour

class Reflex::GLX::SplitBehaviour : public Object::Delegate
{
public:

	REFLEX_OBJECT(GLX::SplitBehaviour, Delegate);

	REFLEX_DECLARE_KEY32(split_size);

	
	static TRef <SplitBehaviour> Create();

	void EnableSplit(GLX::Object & item) { Data::SetBool(item, kresize, true); }

	virtual void ClearSplitSize(GLX::Object & item) = 0;

	virtual void SetSplitSize(GLX::Object & item, Float size) = 0;

	virtual Float GetSplitSize(const GLX::Object & item) const = 0;
};

REFLEX_SET_TRAIT(Reflex::GLX::SplitBehaviour, IsSingleThreadExclusive);

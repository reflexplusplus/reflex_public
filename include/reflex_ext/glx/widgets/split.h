#pragma once

#include "../behaviours/split.h"




//
//Primary API

namespace Reflex::GLX
{

	class Split;

}




//
//Split

class Reflex::GLX::Split : public Object
{
public:

	//types

	REFLEX_OBJECT(GLX::Split,Object);



	//lifetime

	Split();



	//items

	void ClearSplitSize(Object & item) { behaviour->ClearSplitSize(item); }

	void SetSplitSize(Object & item, Float size) { behaviour->SetSplitSize(item, size); }

	Float GetSplitSize(const Object & item) const { return behaviour->GetSplitSize(item); }



	//components

	const TRef <SplitBehaviour> behaviour;

};

REFLEX_SET_TRAIT(Reflex::GLX::Split, IsSingleThreadExclusive);




//
//impl

inline Reflex::GLX::Split::Split()
	: behaviour(SplitBehaviour::Create())
{
	SetDelegate(MakeKey32("SplitBehaviour"), behaviour);
}

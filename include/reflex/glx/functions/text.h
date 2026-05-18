#pragma once

#include "../text.h"
#include "../defines.h"




//
//Primary API

namespace Reflex::GLX
{

	void ClearText(Object & object, Key32 id = kvalue);

	void SetText(Object & object, WString && label, Key32 id = kvalue);

	void SetText(Object & object, const WString & label, Key32 id = kvalue);

	WString::View GetText(const Object & object, Key32 id = kvalue);


	void SetViewPortGridStringifier(Object & object, Key32 id, const Function <WString(Float32 position)> & stringifier);

}




//
//impl

inline void Reflex::GLX::SetText(Object & object, const WString & label, Key32 id)
{
	SetText(object, Copy(label), id);
}


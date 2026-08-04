#pragma once

#include "reflex/glx.h"




//
//Addon API

namespace Reflex::GLX
{

	class Label;

}




//
//Label

class Reflex::GLX::Label : public Object
{
public:

	REFLEX_OBJECT(GLX::Label, Object);



	//lifetime

	Label(const WChar * string = L"", Key32 id = kvalue);

	Label(WString::View string, Key32 id = kvalue);

	Label(const WString & string, Key32 id = kvalue);

	Label(WString && string, Key32 id = kvalue);

	

protected:

	virtual void OnClearValue() {}

	virtual void OnSetValue(const WString & value) {}

};




//
//impl

inline Reflex::GLX::Label::Label(const WChar * string, Key32 id)
	: Label(WString(string), id)
{
}

inline Reflex::GLX::Label::Label(WString::View value, Key32 id)
	: Label(WString(value), id)
{
}

inline Reflex::GLX::Label::Label(const WString & value, Key32 id)
	: Label(Copy(value), id)
{
}

#pragma once

#include "viewport.h"




//
//declarations

namespace Reflex::GLX
{

	template <class CONTENT> class ScrollerOfType;

}




//
//ScrollerOfType

template <class CONTENT>
class Reflex::GLX::ScrollerOfType : public Scroller
{
public:

	//types

	using Base = ScrollerOfType;

	using Content = CONTENT;



	//lifetime

	ScrollerOfType();

	template <class TYPE> ScrollerOfType(TYPE && content);



	//content

	template <class auto_1> CONTENT & SetContent(auto_1 && content);


	TRef <CONTENT> GetContent();

	ConstTRef <CONTENT> GetContent() const;

};




//
//impl

template <class CONTENT> inline Reflex::GLX::ScrollerOfType<CONTENT>::ScrollerOfType()
{
	Scroller::SetContent(New<CONTENT>());
}

template <class CONTENT> template <class TYPE> inline Reflex::GLX::ScrollerOfType<CONTENT>::ScrollerOfType(TYPE && content)
{
	Scroller::SetContent(Deref(content));
}

template <class CONTENT> template <class auto_1> inline CONTENT & Reflex::GLX::ScrollerOfType<CONTENT>::SetContent(auto_1 && content)
{
	auto & ref = Deref(content);

	Scroller::SetContent(ref);

	return ref;
}

template <class CONTENT> inline Reflex::TRef <CONTENT> Reflex::GLX::ScrollerOfType<CONTENT>::GetContent()
{
	if constexpr (IsType<CONTENT,Object>::value)
	{
		return Scroller::GetContent();
	}
	else
	{
		return Cast<CONTENT>(Scroller::GetContent());
	}
}

template <class CONTENT> inline Reflex::ConstTRef <CONTENT> Reflex::GLX::ScrollerOfType<CONTENT>::GetContent() const
{
	if constexpr (IsType<CONTENT,Object>::value)
	{
		return Scroller::GetContent();
	}
	else
	{
		return Cast<CONTENT>(Scroller::GetContent());
	}
}

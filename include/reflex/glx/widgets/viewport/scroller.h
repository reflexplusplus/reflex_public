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
class Reflex::GLX::ScrollerOfType : public ScrollArea
{
public:

	//types

	using Base = ScrollerOfType;

	using Content = CONTENT;



	//lifetime

	ScrollerOfType();

	template <class TYPE> ScrollerOfType(TYPE && content);



	//content

	template <class auto_1> AlreadyRetained <CONTENT> SetContent(auto_1 && content);


	AlreadyRetained <CONTENT> GetContent();

	AlreadyRetained <const CONTENT> GetContent() const;

};




//
//impl

template <class CONTENT> inline Reflex::GLX::ScrollerOfType<CONTENT>::ScrollerOfType()
{
	ScrollArea::SetContent(New<CONTENT>());
}

template <class CONTENT> template <class TYPE> inline Reflex::GLX::ScrollerOfType<CONTENT>::ScrollerOfType(TYPE && content)
{
	ScrollArea::SetContent(Deref(content));
}

template <class CONTENT> template <class auto_1> inline Reflex::AlreadyRetained <CONTENT> Reflex::GLX::ScrollerOfType<CONTENT>::SetContent(auto_1 && content)
{
	auto & ref = Deref(content);

	ScrollArea::SetContent(ref);

	return ref;
}

template <class CONTENT> inline Reflex::AlreadyRetained <CONTENT> Reflex::GLX::ScrollerOfType<CONTENT>::GetContent()
{
	if constexpr (IsType<CONTENT,Object>::value)
	{
		return *ScrollArea::GetContent();
	}
	else
	{
		return *Cast<CONTENT>(ScrollArea::GetContent());
	}
}

template <class CONTENT> inline Reflex::AlreadyRetained <const CONTENT> Reflex::GLX::ScrollerOfType<CONTENT>::GetContent() const
{
	if constexpr (IsType<CONTENT,Object>::value)
	{
		return *ScrollArea::GetContent();
	}
	else
	{
		return *Cast<CONTENT>(ScrollArea::GetContent());
	}
}

#pragma once

#include "../widgets/textarea.h"




//
//Addon API

namespace Reflex::GLX::Detail
{

	void BeginMouseTracking(Object & object, const Function <void(const Point&)> & onmouseover, const Function <void()> & onmouseleave = {});

	void EndMouseTracking(Object & object);


	AlreadyRetained <TextArea> BeginTextEdit_DEPRECATED(Object & object, const Style & style, const WString::View & label, const Function <void(const WString&)> & ondone, const Function <void(const WString&)> & onedit = {}, const Function <void()> & oncancel = {});

	AlreadyRetained <TextEditBehaviour> BeginTextEdit(Object & object, const Function <void(TransactionStage stage, const WString & value)> & onedit, Key32 dataid = kvalue);


	void Show(Object & object);

	void Hide(Object & object);


	void SetContent(Object & parent, Object & content, bool yaxis, bool far);

}




//
//impl

REFLEX_NS(Reflex::GLX::Detail)

void SetContentNotAnimated(Object & parent, Object & content, bool yaxis, bool far);

void SetContentAnimated(Object & parent, Object & content, bool yaxis, bool far);

extern decltype(&SetContentNotAnimated) g_set_content_impl;

REFLEX_END

REFLEX_INLINE void Reflex::GLX::Detail::SetContent(Object & parent, Object & content, bool yaxis, bool far)
{
	g_set_content_impl(parent, content, yaxis, far);
}

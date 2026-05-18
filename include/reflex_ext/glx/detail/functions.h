#pragma once

#include "../widgets/textarea.h"




//
//Addon API

namespace Reflex::GLX::Detail
{

	void BeginMouseTracking(Object & object, const Function <void(const Point&)> & onmouseover, const Function <void()> & onmouseleave = {});

	void EndMouseTracking(Object & object);


	TRef <TextArea> BeginTextEdit_DEPRECATED(Object & object, const Style & style, const WString::View & label, const Function <void(const WString&)> & ondone, const Function <void(const WString&)> & onedit = {}, const Function <void()> & oncancel = {});

	TRef <TextEditBehaviour> BeginTextEdit(Object & object, const Function <void(TransactionStage stage, const WString & value)> & onedit, Key32 dataid = kvalue);


	void Show(Object & object);

	void Hide(Object & object);


	void SetContent(Object & parent, Object & content, bool yaxis, bool far);

}




//
//impl

REFLEX_NS(Reflex::GLX::Detail)

void SetContentAnimated(Object & parent, Object & content, bool yaxis, bool far);

void SetContentNotAnimated(Object & parent, Object & content, bool yaxis, bool far);

constexpr decltype(&SetContentAnimated) kSetContentSwitch[] = { &SetContentNotAnimated, &SetContentAnimated };

REFLEX_END

inline void Reflex::GLX::Detail::SetContent(Object & parent, Object & content, bool yaxis, bool far)
{
	kSetContentSwitch[kCanRenderToTexture](parent, content, yaxis, far);
}

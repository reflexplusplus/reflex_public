#pragma once

#include "reflex/glx.h"




//
//Primary API

namespace Reflex::GLX
{

	class TextArea;

}




//
//Primary API

class Reflex::GLX::TextArea : public ScrollArea
{
public:

	REFLEX_OBJECT(GLX::TextArea, ScrollArea);



	//lifetime

	TextArea(bool multi_line = false, Key32 textid = kvalue);



	//access

	void ClearText();

	void SetText(WString && label);

	void SetText(const WString & label);

	WString::View GetText() const;



	//links

	const TRef <TextEditBehaviour> behaviour;

};




//
//impl

REFLEX_NS(Reflex::GLX)

//setting text on TextArea container is typically unintended error, use textarea.SetText(). Cast to GLX::Object if really intended
void ClearText(TextArea & textarea) = delete;						
void SetText(TextArea & textarea, WString && label, Key32 value = kvalue) = delete;
void SetText(TextArea & textarea, const WString & label, Key32 value = kvalue) = delete;
void SetText(TextArea & textarea, const WString::View & label, Key32 value = kvalue) = delete;
WString::View GetText(const TextArea & textarea) = delete;

REFLEX_END

inline void Reflex::GLX::TextArea::ClearText()
{
	GLX::ClearText(GetContent());

	behaviour->Update();
}

inline void Reflex::GLX::TextArea::SetText(WString && label)
{
	GLX::SetText(GetContent(), std::move(label));

	behaviour->Update();
}

inline void Reflex::GLX::TextArea::SetText(const WString & label)
{
	SetText(Copy(label));
}

inline Reflex::WString::View Reflex::GLX::TextArea::GetText() const
{
	return GLX::GetText(GetContent());
}

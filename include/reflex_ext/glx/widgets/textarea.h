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

class Reflex::GLX::TextArea : public ScrollerOfType <Object>
{
public:

	REFLEX_OBJECT(GLX::TextArea, Scroller);



	//lifetime

	TextArea(bool multi_line = false, Key32 textid = kvalue);



	//access

	void ClearText();

	void SetText(WString && label);

	void SetText(const WString & label);

	WString::View GetText() const;



	//links

	const TRef <TextEditBehaviour> behaviour;



protected:

	void OnSetProperty(Address address, Reflex::Object & object) override;

};




//
//impl

REFLEX_NS(Reflex::GLX)

[[deprecated("use TextArea::ClearText")]] void ClearText(TextArea & object);						//no longer available

[[deprecated("use TextArea::SetText")]] void SetText(TextArea & object, WString && label);

[[deprecated("use TextArea::SetText")]] void SetText(TextArea & object, const WString & label);

[[deprecated("use TextArea::GetText")]] WString::View GetText(const TextArea & object);

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

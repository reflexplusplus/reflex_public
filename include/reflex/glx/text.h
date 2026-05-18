#pragma once

#include "[require].h"




//
//Secondary API (Primary: SetText)

namespace Reflex::GLX
{

	class Text;

}




//
//Text

class Reflex::GLX::Text : public Reflex::Object
{
public:

	REFLEX_OBJECT(Text, Reflex::Object);

	static Text & null;



	//lifetime

	Text(bool multi_line = false) : m_multi_line(multi_line) {}

	Text(const WString::View & value, bool multi_line = false);

	Text(WString && value, bool multi_line = false);



	//content

	void ClearValue();

	void SetValue(WString && value);

	void SetValue(const WString::View & value) { SetValue(WString(value)); }

	WString::View GetView() const { return m_value; }


	bool IsMultiLine() const { return m_multi_line; }



protected:

	WString m_value;

	bool m_multi_line;
};

REFLEX_SET_TRAIT(Reflex::GLX::Text, IsSingleThreadExclusive);

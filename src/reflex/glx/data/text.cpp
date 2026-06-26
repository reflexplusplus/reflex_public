#include "../../../../include/reflex/glx/text.h"




//
//text

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

void AssertMultiLine(const WString::View & text, bool multi_line)
{
#if REFLEX_DEBUG
	if (!multi_line)
	{
		REFLEX_ASSERT(!True(Search(text, WChar(10))));
	}
#endif
}

REFLEX_END_INTERNAL

Reflex::GLX::Text::Text(const WString::View & value, bool multi_line)
	: m_value(value),
	m_multi_line(multi_line)
{
	AssertMultiLine(value, multi_line);
}

Reflex::GLX::Text::Text(WString && value, bool multi_line)
	: m_value(std::move(value)),
	m_multi_line(multi_line)
{
	AssertMultiLine(m_value, multi_line);
}

void Reflex::GLX::Text::ClearValue()
{
	m_value.Clear();
}

void Reflex::GLX::Text::SetValue(WString && value)
{
	m_value = std::move(value);

	AssertMultiLine(m_value, m_multi_line);
}

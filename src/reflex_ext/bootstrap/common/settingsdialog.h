#pragma once

#include "reflex_ext/bootstrap/common/global.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap)

GLX::Form & InitPopup(GLX::Form & form, const GLX::Style & style)
{
	GLX::SetFlow(form, GLX::kFlowX);

	GLX::AddInlineFlex(form, form.body);

	form.body->SetDelegate({}, GLX::PopupBehaviour::Create());

	form.SetStyle(style);

	return form;
}

GLX::Form & InitButton(GLX::Form & form, const GLX::Style & style, Key32 id)
{
	GLX::SetFlow(form, GLX::kFlowX);

	GLX::AddInlineFlex(form, form.body);

	form.body->SetMouseCursor(GLX::kMouseCursorPointer);

	form.SetStyle(style);

	form.body->id = id;

	return form;
}

struct SettingsPanel : public IDE::Detail::ConsolePanel
{
	static inline SettingsPanel * st_self = 0;

	SettingsPanel();

	void OnReset(Key32 context) override;

	void OnRestore(Data::Archive::View & stream, Key32 context) override;

	void OnStore(Data::Archive & stream) const override;


	ConstReference <GLX::Style> m_stylesheet;

	GLX::TabGroup m_tabgroup;

	GLX::Button m_logfile;

	GLX::Button m_clear_preferences;

	GLX::Object m_footer;
};

REFLEX_END_INTERNAL

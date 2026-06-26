#pragma once

#include "reflex/glx/behaviours/textedit.h"
#include "reflex/glx/text.h"
#include "reflex/glx/detail/font.h"




//
//core

REFLEX_NS(Reflex::GLX)

struct TextEditBehaviourWithState : public TextEditBehaviour
{
	REFLEX_OBJECT(TextEditBehaviourWithState, TextEditBehaviour);

	TextEditBehaviourWithState(Key32 id);


	virtual void RequestTextInput() {}	//additional internal callback


	void SetInputType(VirtualKeyboardInputType type) override {}

	void SetTabSpaces(UInt8 spaces) override {}

	void SetCaret(UInt pos, UInt selection_start, UInt selection_length) override {}

	Tuple <UInt, UInt, UInt> GetCaret() const override { return { m_caret, m_selection.a, m_selection.b }; }

	void Update() override {}

	void Reveal() override {}

	Float GetLineHeight() const override { return {}; }

	Pair <Float> GetLineCoordinates(UInt idx) const override { return {}; }

	void OnRestoreHistory(Data::Archive::View stream, bool redo) override {}



	const Key32 m_textid;

	Reference <Text> m_text;

	VirtualKeyboardInputType m_input_type;

	bool m_tab_spaces = true;

	mutable UInt8 m_compute_scope_count = 0;

	bool m_show_caret;

	UInt16 m_caret;

	Pair <UInt16> m_selection;

	Reference <Reflex::Object> m_virtual_keyboard;	//mobile keyboard management


	//shared with and set by TextEdit layer

	ConstReference <Detail::Font> m_font;

	Rect m_text_rect;

	Float32 m_lineh;

	ConstReference <Data::WStringProperty> m_transformed;
};

REFLEX_END

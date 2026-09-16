#pragma once

#include "form.h"




//
//Primary API

namespace Reflex::GLX
{

	class FlowDialog;

}




//
//FlowDialog

class Reflex::GLX::FlowDialog final : public FormEx
{
public:

	REFLEX_OBJECT(GLX::FlowDialog, FormEx);

	REFLEX_GLX_EVENT_ID(ShowPage);					//@Key32 id

	static constexpr Key32 kDisabled = kNullKey;	//special prev_id/next_id to "grey out" button
	static constexpr Key32 kExit = kZeroKey;		//special prev_id/next_id to close dialog

	static constexpr WString::View kBackLabel = L"Back";
	static constexpr WString::View kNextLabel = L"Next";

	enum Direction : UInt8
	{
		kButtonBack = 0,
		kButtonNext = 1,

		kDirectionBack = kButtonBack,
		kDirectionForward = kButtonNext
	};

	struct Labels
	{
		WString prev = kBackLabel;

		WString next = kNextLabel;
	};



	//lifetime

	FlowDialog(const WString::View & title);



	//setup

	void Clear();

	void RegisterPage(Key32 id, Key32 prev_id, Key32 next_id, Key32 style_id, const Labels & labels, Function <Unretained<Object>(FlowDialog&)> ctr);
	
	void RegisterPage(Key32 id, Key32 prev_id, Key32 next_id, const Labels & labels, Function <Unretained<Object>(FlowDialog&)> ctr) { RegisterPage(id, prev_id, next_id, id, labels, ctr); }

	void UpdateButton(Key32 page_id, Direction button, Key32 button_page_id);



	//access

	bool ShowPage(Key32 id, Direction direction = kDirectionForward);

	AlreadyRetained <Object> GetCurrentPage();



	//close

	void Close();



protected:

	void OnSetStyle(const Style & style) override;

	bool OnEvent(Object & src, Event & e) override;



private:

	struct Page
	{
		Labels labels;

		Key32 prev_id, next_id;

		Key32 style_id;

		Function <Unretained<Object>(FlowDialog&)> ctr;
	};

	Page * QueryPage(Key32 page_id);

	void UpdateButtons(const Page & page);



	ConstAlreadyRetained <Style> m_body_style;

	Button m_buttons[2];


	Map <Key32,Page> m_pages;

	Key32 m_current_page, m_current_style;

	AlreadyRetained <Object> m_content;

};




//
//impl

inline Reflex::AlreadyRetained <Reflex::GLX::Object> Reflex::GLX::FlowDialog::GetCurrentPage()
{
	return m_content;
}

inline void Reflex::GLX::FlowDialog::Close()
{
	EmitCloseRequest(*this);
}

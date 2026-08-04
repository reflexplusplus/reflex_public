#pragma once

#include "../[require].h"




#define REFLEX_EXPOSE_MEMBER_METHOD(MEMBER,METHOD) template <typename... VARGS> decltype(auto) METHOD(VARGS && ... args) { return Reflex::Deref(MEMBER).METHOD(std::forward<VARGS>(args)...); }

#define REFLEX_EXPOSE_MEMBER_METHOD_CONST(MEMBER,METHOD) template <typename... VARGS> decltype(auto) METHOD(VARGS && ... args) const { return Reflex::Deref(MEMBER).METHOD(std::forward<VARGS>(args)...); }




//
//Detail

namespace Reflex::IDE::Detail
{

	class TextEditor;

}




//
//TextEditor

class Reflex::IDE::Detail::TextEditor : public GLX::Object
{
public:

	REFLEX_DECLARE_KEY32(Edit);



	//lifetime

	TextEditor();



	//setup

	void Enable(bool enable);



	//access

	void ClearData();

	void SetData(ConstTRef <Data::ArchiveObject> archive);

	ConstTRef <Data::ArchiveObject> GetData() const { return m_data; }


	REFLEX_EXPOSE_MEMBER_METHOD(m_textedit, SetCaret);

	REFLEX_EXPOSE_MEMBER_METHOD_CONST(m_textedit, GetCaret);


	REFLEX_EXPOSE_MEMBER_METHOD(m_textedit, Reset);

	REFLEX_EXPOSE_MEMBER_METHOD(m_textedit, Deserialize);

	REFLEX_EXPOSE_MEMBER_METHOD_CONST(m_textedit, Serialize);


	void Reveal(UInt pos, UInt length);


	//void SetCommentedLines(const ArrayView <UInt> & lines);


	void ClearError();

	void SetError(UInt idx);



private:

	virtual bool OnEvent(GLX::Object & src, GLX::Event & e) override;

	GLX::Rect GetLineCoordinates(UInt idx) const;


	const TRef <GLX::Text> m_text;

	TRef <GLX::TextEditBehaviour> m_textedit;

	ConstReference <Data::ArchiveObject> m_data;


	Idx m_highlighted_line;

	//Array <UInt32> m_commented_lines;
};

#pragma once

#include "label.h"




//
//Addon API

namespace Reflex::GLX
{

	class Form;

	class FormEx;

}




//
//Form

class Reflex::GLX::Form : public Object
{
public:

	REFLEX_OBJECT(GLX::Form, Object);

	
	
	//style ids

	REFLEX_DECLARE_KEY32(header);
	REFLEX_DECLARE_KEY32(body);



	//lifetime

	Form(TRef <Object> body = New<Object>());

	Form(const WString::View & label, TRef <Object> body = New<Object>());

	~Form();



	//elements

	const TRef <Label> header;

	const TRef <Object> body;



protected:

	virtual void OnSetStyle(const Style & style) override;

};




//
//FormEx

class Reflex::GLX::FormEx : public Form
{
public:

	//style ids

	REFLEX_DECLARE_KEY32(footer);



	//lifetime
	
	FormEx(const WString::View & title, TRef <Object> body = New<Object>(), TRef <Object> footer = New<Object>());

	~FormEx();



	//elements

	const TRef <Object> footer;



protected:

	virtual void OnSetStyle(const Style & style) override;

};

#pragma once

#include "../../common/ui/[require].h"
#include "../parameter.h"




//
//Primary API

namespace Reflex::Bootstrap
{

	class GenericControl;

}




//
//GenericControl

class Reflex::Bootstrap::GenericControl : public GLX::Object
{
public:

	REFLEX_OBJECT(Bootstrap::GenericControl, GLX::Object);

	REFLEX_USE_ENUM(ParameterDefinition, Type);



	GenericControl();


	Type GetType() const { return m_type_active.a; }

	AlreadyRetained <GLX::Object> GetContent() const { return m_content; }



protected:

	void SetLabel(const WString::View & label);

	void SetValueText(const WString::View & value);

	void ClearWidget();

	AlreadyRetained <GLX::Object> AcquireWidget(Type type, bool active = true);



private:

	void OnSetStyle(const GLX::Style & style) override;


	ConstReference <Reflex::Object> m_cstyle;

	Pair <Type,bool> m_type_active;

	Key32 m_type_state;

	AlreadyRetained <GLX::Object> m_content;

	Reference <GLX::Text> m_value;

};

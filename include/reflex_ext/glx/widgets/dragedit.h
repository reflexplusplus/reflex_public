#pragma once

#include "rotaryslider.h"




//
//Addon API

namespace Reflex::GLX
{

	class DragEdit;

}




//
//DragEdit

class Reflex::GLX::DragEdit : public RotarySlider
{
public:

	REFLEX_OBJECT(GLX::DragEdit, RotarySlider);



	//lifetime

	DragEdit();

	~DragEdit();



	//setup

	void SetTextConverter(const Function <WString(Float)> & value2text, const Function <Float(const WString&)> & text2value);



	//setup

	virtual void SetSensitivity(Float factor) override;

	void SetPrecision(UInt precision);


	virtual bool SetRange(Float min, Float max, Float step = 0.0f) override;



	//value

	virtual bool SetValue(Float value) override;



protected:

	virtual void OnUpdate() override;



private:

	Function <WString(Float)> m_value2text;

	Function <Float(const WString&)> m_text2value;


	Float32 m_sensitivity_factor;

	Text m_text;

};

REFLEX_SET_TRAIT(Reflex::GLX::DragEdit, IsSingleThreadExclusive);

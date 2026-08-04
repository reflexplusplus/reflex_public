#pragma once

#include "reflex/glx.h"




//
//Addon API

namespace Reflex::GLX
{

	class RotarySlider;


	TRef <Object::Delegate> AttachRotaryDisplayPropertiesDelegate(GLX::Object & object, Float value_origin = 0.0f, Key32 indicator_id = MakeKey32("indicator"), Key32 sweep_id = MakeKey32("sweep"));

	Tuple <Float,Range> CalcRotaryAngleAndSweep(Range value_range, Float value, Float value_origin, Range sweep_range = { 0.375f, 0.75f });

}




//
//RotarySlider (TODO -> Behaviour)

class Reflex::GLX::RotarySlider : public Object
{
public:

	REFLEX_OBJECT(GLX::RotarySlider, Object);



	//properties

	static constexpr auto krange = GLX::krange;		//RangeProperty

	static constexpr auto kvalue = GLX::kvalue;		//Data::Float32Property



	//events

	static constexpr auto kTransaction = GLX::kTransaction;

	REFLEX_GLX_EVENT_ID(DoubleClick);



	//lifetime

	RotarySlider();



	//setup

	virtual void SetSensitivity(Float pixels);


	virtual bool SetRange(Float min, Float max, Float step = 0.0f);

	Tuple <Range,Float> GetRange() const;


	virtual void SetDefault(Float value);

	Float GetDefault() const;



	//access

	void Reset();


	virtual bool SetValue(Float value);

	Float GetValue() const;



protected:

	bool OnEvent(Object & src, Event & e) override;

	Core::Trap OnPointerTender(Core::PointerAction action, const Core::Pointer & pointer, UInt8 flags) override;

	void OnDetachWindow() override;

	void OnSetProperty(Address adr, Reflex::Object & object) override;

	void OnQueryProperty(Address adr, Reflex::Object * & pobject) const override;


	void SetValueTransacted(Float32 value);



private:

	Float m_sensitivity, m_quantise;

	Float m_default;


	Reference <RangeProperty> m_range;

	Reference <Data::Float32Property> m_value;

};

REFLEX_SET_TRAIT(Reflex::GLX::RotarySlider, IsSingleThreadExclusive);




//
//impl

inline Reflex::Tuple <Reflex::GLX::Range, Reflex::Float> Reflex::GLX::RotarySlider::GetRange() const
{
	return { m_range->value, m_quantise };
}

inline Reflex::Float Reflex::GLX::RotarySlider::GetDefault() const
{
	return m_default;
}

#pragma once

#include "generic_control.h"




//
//Primary API

namespace Reflex::Bootstrap
{

	class ParameterControl;	//widget to edit a parameter

	using ParamControl = ParameterControl;

}




//
//ParameterControl

class Reflex::Bootstrap::ParameterControl : public GenericControl
{
public:

	struct ParameterInterface;

	[[nodiscard]] static Unretained <ParameterControl> Create(AudioPlugin & instance, UInt param_idx);



protected:

	ParameterControl();

	~ParameterControl();


	void Bind(const ParameterInterface & adapter, WillRetain <Reflex::Object> state, ConstWillRetain <ParameterDefinition> definition, UInt index = 0, bool enabled = true);

	void Unbind();

	void SetControlSensitivity(Float32 factor);

	void SetValueNormalizedAtomic(Float32 value, bool fine = false);


	ConstAlreadyRetained <ParameterDefinition> GetDefinition() const { return m_definition; }

	AlreadyRetained <Reflex::Object> GetBindingState() const { return m_state.Adr(); }

	UInt GetBindingIndex() const { return m_index; }


	bool OnEvent(GLX::Object & src, GLX::Event & e) override;

	void OnClock(Float32 delta) override;

	void OnUpdate() override;



private:

	void BeginControlEdit();

	void PerformControlEdit(Float32 value, bool fine);

	void EndControlEdit(bool cancel);

	Float32 ValueToNormal(Float32 value) const;


	const ParameterInterface * m_adapter;

	Reference <Reflex::Object> m_state;

	ConstReference <ParameterDefinition> m_definition;

	UInt16 m_index;

	bool m_editing;

	bool m_binding_enabled;

	Float32 m_sensitivity;

	Value32 m_value_z;

};

REFLEX_SET_TRAIT(Bootstrap::ParameterControl, IsAbstract);




//
//ParameterControl::ParameterInterface

struct Reflex::Bootstrap::ParameterControl::ParameterInterface
{
	using Object = Reflex::Object;
	using GetValueFn = FunctionPointer <Value32(const Object & state, UInt index)>;
	using ToStringFn = FunctionPointer <WString(const Object & state, UInt index, Value32 value)>;
	using BeginEditFn = FunctionPointer <Unretained<Object>(ParameterControl & control, Object & state, UInt index)>;
	using PerformEditFn = FunctionPointer <void(ParameterControl & control, Object & state, UInt index, Object & transaction, Float32 value, bool fine)>;
	using EndEditFn = FunctionPointer <void(ParameterControl & control, Object & state, UInt index, Object & transaction, bool cancel)>;

	GetValueFn get_value;
	ToStringFn to_string;
	BeginEditFn begin_edit;
	PerformEditFn perform_edit;
	EndEditFn end_edit;
};

#include "../../../../include/reflex_ext/bootstrap/audioplugin/ui/parameter_control.h"




//
//adapters

REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap)

const ParameterControl::ParameterInterface kAudioPluginAdapter =
{
	.get_value = [](const Object & state, UInt index)
	{
		auto instance = Cast<AudioPlugin>(state);

		return instance->GetParameterValues()[index];
	},
	.to_string = [](const Object & state, UInt index, Value32 value)
	{
		auto definition = Cast<AudioPlugin>(state)->GetParameterInfo(index);

		return definition->ToString(value);
	},
	.begin_edit = [](ParameterControl &, Object & state, UInt index) -> Unretained <Object>
	{
		Cast<AudioPlugin>(state)->BeginAutomation(index);

		return Unretained<Object>(Object::null);
	},
	.perform_edit = [](ParameterControl &, Object & state, UInt index, Object &, Float32 value, bool)
	{
		Cast<AudioPlugin>(state)->Automate(index, value);
	},
	.end_edit = [](ParameterControl &, Object & state, UInt index, Object &, bool)
	{
		Cast<AudioPlugin>(state)->EndAutomation(index);
	},
};

REFLEX_END_INTERNAL

Reflex::Unretained <Reflex::Bootstrap::ParameterControl> Reflex::Bootstrap::ParameterControl::Create(AudioPlugin & instance, UInt param_idx)
{
	auto control = REFLEX_CREATE(ParameterControl);

	control->Bind(kAudioPluginAdapter, instance, instance.GetParameterInfo(param_idx), param_idx);

	return control;
}

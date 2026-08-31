#include "../../../../include/reflex_ext/bootstrap/audioplugin/ui/parameter_control.h"




//
//null adapter

REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap)

constexpr UInt32 kControlTransaction = K32("{ControlTransaction}");

const ParameterControl::ParameterInterface kNullAdapter =
{
	.get_value = [](const Object &, UInt)
	{
		return 0.0f;
	},
	.to_string = [](const Object &, UInt, Float32) -> WString
	{
		return {};
	},
	.begin_edit = [](ParameterControl &, Object &, UInt)
	{
		return Null<Object>();
	},
	.perform_edit = [](ParameterControl &, Object &, UInt, Object &, Float32, bool)
	{
	},
	.end_edit = [](ParameterControl &, Object &, UInt, Object &, bool)
	{
	},
};

REFLEX_END_INTERNAL

Reflex::Bootstrap::ParameterControl::ParameterControl()
	: m_adapter(&kNullAdapter)
	, m_state(REFLEX_NULL(Object))
	, m_definition(REFLEX_NULL(ParameterDefinition))
	, m_index(0)
	, m_editing(false)
	, m_binding_enabled(false)
	, m_sensitivity(1.0f)
	, m_value_z(-1.0f)
{
	EnableOnClock();

	Update();
}

Reflex::Bootstrap::ParameterControl::~ParameterControl()
{
	EndControlEdit(true);
}

void Reflex::Bootstrap::ParameterControl::Bind(const ParameterInterface & adapter, TRef <Reflex::Object> state, ConstTRef <ParameterDefinition> definition, UInt index, bool enabled)
{
	EndControlEdit(true);

	m_adapter = &adapter;
	m_state = state;
	m_definition = definition;
	m_index = index;
	m_binding_enabled = enabled;
	m_value_z = -1.0f;

	Update();
}

void Reflex::Bootstrap::ParameterControl::Unbind()
{
	EndControlEdit(true);

	m_adapter = &kNullAdapter;
	m_state = REFLEX_NULL(Object);
	m_index = 0;
	m_definition = REFLEX_NULL(ParameterDefinition);
	m_value_z = -1.0f;
	m_binding_enabled = false;

	Update();
}

void Reflex::Bootstrap::ParameterControl::SetControlSensitivity(Float32 factor)
{
	m_sensitivity = factor;

	Update();
}

void Reflex::Bootstrap::ParameterControl::BeginControlEdit()
{
	if (SetFiltered(m_editing, true))
	{
		SetAbstractProperty(*this, kControlTransaction, m_adapter->begin_edit(*this, m_state, m_index));

		SetState(GLX::kActiveState);
	}
}

void Reflex::Bootstrap::ParameterControl::PerformControlEdit(Float32 value, bool fine)
{
	m_adapter->perform_edit(*this, m_state, m_index, GetAbstractProperty(*this, kControlTransaction), Clip(value, 0.0f, 1.0f), fine);
}

void Reflex::Bootstrap::ParameterControl::EndControlEdit(bool cancel)
{
	if (SetFiltered(m_editing, false))
	{
		m_adapter->end_edit(*this, m_state, m_index, GetAbstractProperty(*this, kControlTransaction), cancel);

		UnsetAbstractProperty(*this, kControlTransaction);
	}
}

void Reflex::Bootstrap::ParameterControl::SetValueNormalizedAtomic(Float32 value, bool fine)
{
	BeginControlEdit();

	PerformControlEdit(value, fine);

	EndControlEdit(false);
}

Reflex::Float32 Reflex::Bootstrap::ParameterControl::ValueToNormal(Float32 value) const
{
	TRef definition = m_definition;
	
	Value32 native;

	if (definition->GetType() == ParameterDefinition::kTypeContinuous)
	{
		native.fvalue = value;
	}
	else
	{
		native.ivalue = ToInt32(value);
	}

	return Bootstrap::Normalise(definition, native);
}

Reflex::Float32 Reflex::Bootstrap::ParameterControl::NormalToValue(Float32 normal) const
{
	TRef definition = m_definition;

	auto native = Expand(definition, normal);

	return definition->GetType() == ParameterDefinition::kTypeContinuous ? native.fvalue : Float32(native.ivalue);
}

bool Reflex::Bootstrap::ParameterControl::OnEvent(GLX::Object & src, GLX::Event & e)
{
	if (e.id == GLX::kTransaction && src == GetContent())
	{
		auto value = ValueToNormal(Cast<GLX::RotarySlider>(src)->GetValue());

		switch (GLX::GetTransactionStage(e))
		{
		case GLX::kTransactionStageBegin:
			BeginControlEdit();
			break;

		case GLX::kTransactionStagePerform:
			PerformControlEdit(value, GLX::GetModifierKeys(e) & GLX::kModifierKeyShift);
			break;

		case GLX::kTransactionStageCancel:
			EndControlEdit(true);
			break;

		case GLX::kTransactionStageEnd:
			EndControlEdit(false);
			break;
		}

		return true;
	}
	else if (e.id == GLX::kMouseDown && src == GetContent() && GetType() == kTypeBoolean && !(GLX::GetClickFlags(e) & GLX::kClickFlagRmb))
	{
		SetValueNormalizedAtomic(m_adapter->get_value(m_state, m_index) < 0.5f ? 1.0f : 0.0f);

		return true;
	}
	else if (auto menu = GLX::GetMenu(e); menu && GetType() == kTypeEnumeration && src == GetContent())
	{
		menu->Clear();

		auto [min, max] = m_definition->GetRange();

		auto count = UInt(max.ivalue - min.ivalue) + 1;

		REFLEX_LOOP(idx, count)
		{
			auto value = count > 1 ? Float32(idx) / Float32(count - 1) : 0.0f;

			GLX::BindClick(menu->AddItem(m_adapter->to_string(m_state, m_index, value)), BindMethod(this, &ParameterControl::SetValueNormalizedAtomic, value, false));
		}

		return true;
	}

	return GenericControl::OnEvent(src, e);
}

void Reflex::Bootstrap::ParameterControl::OnClock(Float32)
{
	auto value = m_adapter->get_value(m_state, m_index);

	if (SetFiltered(m_value_z, value))
	{
		auto control = GetContent();

		switch (GetType())
		{
		case kTypeContinuous:
		case kTypeDiscrete:
			Cast<GLX::RotarySlider>(control)->SetValue(NormalToValue(value));
			break;

		case kTypeBoolean:
			GLX::Select(control, value >= 0.5f);
			break;

		case kTypeEnumeration:
			break;
		}

		SetValueText(m_adapter->to_string(m_state, m_index, value));

		control->Accommodate();
	}
}

void Reflex::Bootstrap::ParameterControl::OnUpdate()
{
	auto definition = m_definition.Adr();
	auto [min, max] = definition->GetRange();
	auto step = definition->GetStep();
	const bool active = m_binding_enabled;

	auto control = AcquireWidget(GenericControl::Type(definition->GetType()), active);

	GLX::EnableMouse(control, active);
	GLX::EnableMouse(*this, active, !active);
	GLX::SetState(*this, GLX::kInactiveState, !active);

	SetLabel(definition->GetName());

	if (active)
	{
		switch (GetType())
		{
		case kTypeContinuous:
		{
			auto rotary = Cast<GLX::RotarySlider>(control);

			rotary->SetSensitivity(128.0f * m_sensitivity);
			rotary->SetRange(min.fvalue, max.fvalue, step.fvalue);
			rotary->SetDefault(definition->GetDefaultValue().fvalue);
		}
		break;

		case kTypeDiscrete:
		{
			auto drag_edit = Cast<GLX::DragEdit>(control);

			drag_edit->SetSensitivity(4.0f * m_sensitivity);
			drag_edit->SetRange(Float32(min.ivalue), Float32(max.ivalue), Float32(step.ivalue));
			drag_edit->SetDefault(Float32(definition->GetDefaultValue().ivalue));
		}
		break;

		case kTypeBoolean:
		case kTypeEnumeration:
			break;
		}

		EnableOnClock(true);

		OnClock(0.0f);
	}
	else
	{
		EnableOnClock(false);
	}
}

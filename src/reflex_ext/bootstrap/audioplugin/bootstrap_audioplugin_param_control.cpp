#include "reflex_ext/bootstrap/audioplugin/ui/control.h"




//
//ParamControl

REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap)

struct ControlImpl : public ParamControl
{
	ControlImpl(const ParamDesc & param_desc, const Value32 & value);

	void Init(GLX::Object & widget, Key32 stateid);

	void SetValueNormalized(Float fvalue);

	void OnSetStyle(const GLX::Style & style) override;

	void OnUpdate() override;

	void OnClock(Float32 delta) override;


	ConstReference <ParamDesc> m_paraminfo;

	const Value32 & value;

	Value32 m_value_z;
	
	Reference <GLX::Text> m_text;
};

struct DummyControl : public ControlImpl
{
	DummyControl(const ParamDesc & param_desc)
		: ControlImpl(param_desc, m_null_value),
		m_null_value({ Reinterpret<Float32>(kMaxUInt32) })
	{
		GLX::AddFloat(*this, m_rotary, GLX::kAlignmentTop);

		GLX::Activate(m_rotary, false);

		EnableOnClock(false);

		SetState(MakeKey32("continuous"));
	}

	const Value32 m_null_value;
	
	GLX::Object m_rotary;
};

template <bool DISCRETE> 
struct RotaryControl : public ControlImpl
{
	RotaryControl(const ParamDesc & param_desc, const Value32 & value)
		: ControlImpl(param_desc, value)
	{
		GLX::AttachRotaryDisplayPropertiesDelegate(m_rotary);

		auto forigin = param_desc.origin.fvalue;

		if constexpr (DISCRETE)
		{
			forigin = Float32(param_desc.origin.ivalue);

			m_rotary.SetRange(Float32(param_desc.min.ivalue), Float32(param_desc.max.ivalue), 1.0f);
		}
		else
		{
			m_rotary.SetRange(param_desc.min.fvalue, param_desc.max.fvalue, 0.0f);
		}

		Data::SetFloat32(m_rotary, MakeKey32("origin"), forigin);

		Init(m_rotary, MakeKey32("continuous"));
	}

	bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (e.id == GLX::kTransaction && src == m_rotary)
		{
			auto stage = GLX::GetTransactionStage(e);

			auto fvalue = m_rotary.GetValue();

			Value32 value = { DISCRETE ? Reinterpret<Float32>(Truncate(fvalue)) : fvalue };

			GLX::EmitTransaction(*this, stage, 0, Normalise(m_paraminfo, value));

			GLX::SetState(*this, GLX::kActiveState, stage == GLX::kTransactionStageBegin || stage == GLX::kTransactionStagePerform);

			return true;
		}

		return GLX::Object::OnEvent(src, e);
	}

	void OnUpdate() override
	{
		ControlImpl::OnUpdate();

		if constexpr (DISCRETE)
		{
			auto fvalue = Float32(value.ivalue);

			m_rotary.SetValue(fvalue);
		}
		else
		{
			m_rotary.SetValue(value.fvalue);
		}
	}


	GLX::RotarySlider m_rotary;
};

struct ButtonControl : public ControlImpl
{
	ButtonControl(const ParamDesc & param_desc, const Value32 & value)
		: ControlImpl(param_desc, value)
	{
		Init(m_button, MakeKey32("boolean"));
	}

	bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (e.id == GLX::kMouseDown && src == m_button)
		{
			SetValueNormalized(Float32((value.ivalue + 1) & 1));

			return true;
		}

		return GLX::Object::OnEvent(src, e);
	}

	void OnUpdate() override
	{
		ControlImpl::OnUpdate();

		GLX::Select(m_button, True(value.ivalue));
	}

	
	GLX::Button m_button;
};

struct PopupControl : public ControlImpl
{
	PopupControl(const ParamDesc & param_desc, const Value32 & value)
		: ControlImpl(param_desc, value)
	{
		Init(m_popup, MakeKey32("enumeration"));
	}

	bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (auto menu = GLX::GetMenu(e))
		{
			menu->Clear();

			auto max = m_paraminfo->max.ivalue;

			auto n = UInt(max + 1);

			auto fmax = Float32(max);

			Float fidx = 0.0f;

			REFLEX_LOOP(idx, n)
			{
				Value32 value;
				
				value.ivalue = idx;

				GLX::BindClick(menu->AddItem(m_paraminfo->to_string(value)), BindMethod(this, &PopupControl::SetValueNormalized, fidx / fmax));

				fidx++;
			}

			return true;
		}

		return GLX::Object::OnEvent(src, e);
	}


	GLX::Popup m_popup;
};

ControlImpl::ControlImpl(const ParamDesc & param_desc, const Value32 & value)
	: m_paraminfo(param_desc),
	value(value),
	m_value_z({ Reinterpret<Float32>(kMaxUInt32) }),
	m_text(kNewObject)
{
	GLX::SetFlow(*this, GLX::kFlowY);

	GLX::SetText(*this, ToWString(param_desc.name), MakeKey32("label"));

	SetProperty(GLX::kvalue, m_text);

	EnableOnClock();
}

void ControlImpl::Init(GLX::Object & widget, Key32 state_id)
{
	widget.SetProperty(GLX::kvalue, m_text);

	GLX::AddFloat(*this, widget, GLX::kAlignmentTop);

	SetState(state_id);
}

void ControlImpl::OnClock(Float32 delta)
{
	if (SetFiltered(m_value_z.ivalue, value.ivalue))
	{
		Update();
	}
}

void ControlImpl::OnUpdate()
{
	m_text->SetValue(m_paraminfo->to_string(value));

	Accommodate();

	GetFirst()->Accommodate();
}

void ControlImpl::OnSetStyle(const GLX::Style & style)
{
	GetFirst()->SetStyle(style[GLX::kcontent]);
}

void ControlImpl::SetValueNormalized(Float fvalue)
{
	GLX::EmitTransaction(*this, GLX::kTransactionStageBegin, 0, fvalue);

	GLX::EmitTransaction(*this, GLX::kTransactionStagePerform, 0, fvalue);

	GLX::EmitTransaction(*this, GLX::kTransactionStageEnd, 0, fvalue);
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::Bootstrap::ParamControl> Reflex::Bootstrap::ParamControl::Create(ConstTRef <ParamDesc> param_desc, const Value32 & value)
{
	TRef <ParamControl> control = kNoValue;

	switch (param_desc->type)
	{
	case ParamDesc::kTypeBool:
		control = New<ButtonControl>(param_desc, value);
		break;

	case ParamDesc::kTypeEnum:
		control = New<PopupControl>(param_desc, value);
		break;

	case ParamDesc::kTypeDiscrete:
		control = New<RotaryControl<true>>(param_desc, value);
		break;

	default:
		control = New<RotaryControl<false>>(param_desc, value);
		break;
	}

	return control;
}

Reflex::TRef <Reflex::Bootstrap::ParamControl> Reflex::Bootstrap::ParamControl::Create(AudioPlugin & instance, UInt paramidx)
{
	auto control = Create(instance.GetParameterInfo(paramidx), instance.GetParameterValues()[paramidx]);

	GLX::SetEventDelegate(control, {}, [&instance, paramidx](GLX::Object & src, GLX::Event & e)
	{
		switch (GLX::GetTransactionStage(e))
		{
		case GLX::kTransactionStageBegin:
			instance.BeginAutomation(paramidx);
			break;

		case GLX::kTransactionStagePerform:
			instance.Automate(paramidx, Data::GetFloat32(e, GLX::kvalue));
			break;

		case GLX::kTransactionStageEnd:
		case GLX::kTransactionStageCancel:
			instance.EndAutomation(paramidx);
			break;
		}

		return true;
	});

	return control;
}
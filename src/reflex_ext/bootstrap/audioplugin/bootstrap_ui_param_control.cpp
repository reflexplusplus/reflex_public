#include "reflex_ext/bootstrap/audioplugin/ui/control.h"




//
//ParamControl

REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap)

struct ControlImpl : public ParamControl
{
	ControlImpl(const ParamDesc & param_desc, const Value32 & value, Key32 styleid);

	void SetValueNormalized(Float fvalue);

	void OnSetStyle(const GLX::Style & style) override;

	void OnClock(Float32 delta) override;


	const Key32 m_styleid;

	ConstReference <ParamDesc> m_paraminfo;

	const Value32 & value;

	Value32 m_value_z;
};

struct DummyControl : public ControlImpl
{
	DummyControl(const ParamDesc & param_desc)
		: ControlImpl(param_desc, m_null_value, K32("continuous")),
		m_null_value({ Reinterpret<Float32>(kMaxUInt32) })
	{
		GLX::AddFloat(*this, m_rotary, GLX::kAlignmentTop);

		GLX::Activate(m_rotary, false);

		EnableOnClock(false);
	}


	const Value32 m_null_value;
	
	GLX::Object m_rotary;
};

template <bool DISCRETE> 
struct RotaryControl : public ControlImpl
{
	RotaryControl(const ParamDesc & param_desc, const Value32 & value)
		: ControlImpl(param_desc, value, K32("continuous"))
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

		Data::SetFloat32(m_rotary, K32("origin"), forigin);

		GLX::AddFloat(*this, m_rotary, GLX::kAlignmentTop);
	}

	bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (e.id == GLX::kTransaction && src == m_rotary)
		{
			auto stage = GLX::GetTransactionStage(e);

			auto fvalue = m_rotary.GetValue();

			Value32 value = { DISCRETE ? Reinterpret<Float32>(Truncate(fvalue)) : fvalue };

			GLX::EmitTransaction(*this, stage, 0, Normalise(m_paraminfo, value));

			return true;
		}

		return GLX::Object::OnEvent(src, e);
	}

	void OnUpdate() override
	{
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
		: ControlImpl(param_desc, value, K32("boolean"))
	{
		GLX::AddFloat(*this, m_button, GLX::kAlignmentTop);
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
		bool tf = True(value.ivalue);

		GLX::Select(m_button, tf);

		GLX::SetText(m_button, kBooleanLabels[tf]);
	}

	
	GLX::Button m_button;

	static constexpr WString::View kBooleanLabels[] = { L"Off", L"On" };
};

struct PopupControl : public ControlImpl
{
	PopupControl(const EnumParamDesc & param_desc, const Value32 & value)
		: ControlImpl(param_desc, value, K32("enumeration"))
	{
		GLX::AddFloat(*this, m_popup, GLX::kAlignmentTop);
	}

	bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (auto menu = GLX::GetMenu(e))
		{
			menu->Clear();

			auto info = Cast<EnumParamDesc>(*m_paraminfo);

			auto fmax = Float32(info->max.ivalue);

			Float fidx = 0.0f;

			for (auto & i : info->values)
			{
				GLX::BindClick(menu->AddItem(ToWString(i)), BindMethod(this, &PopupControl::SetValueNormalized, fidx / fmax));

				fidx++;
			}

			return true;
		}

		return GLX::Object::OnEvent(src, e);
	}

	void OnUpdate() override
	{
		auto info = Cast<EnumParamDesc>(m_paraminfo);

		GLX::SetText(m_popup, ToWString(info->values[value.ivalue]));
	}


	GLX::Popup m_popup;
};

ControlImpl::ControlImpl(const ParamDesc & param_desc, const Value32 & value, Key32 styleid)
	: m_styleid(styleid),
	m_paraminfo(param_desc),
	value(value),
	m_value_z({ Reinterpret<Float32>(kMaxUInt32) })
{
	GLX::SetFlow(*this, GLX::kFlowY);

	GLX::SetText(*this, ToWString(param_desc.name));

	EnableOnClock();
}

void ControlImpl::OnClock(Float32 delta)
{
	if (SetFiltered(m_value_z.ivalue, value.ivalue))
	{
		Update();
	}
}

void ControlImpl::OnSetStyle(const GLX::Style & style)
{
	GetFirst()->SetStyle(style[m_styleid]);
}

void ControlImpl::SetValueNormalized(Float fvalue)
{
	GLX::EmitTransaction(*this, GLX::kTransactionBegin, 0, fvalue);

	GLX::EmitTransaction(*this, GLX::kTransactionPerform, 0, fvalue);

	GLX::EmitTransaction(*this, GLX::kTransactionEnd, 0, fvalue);
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::Bootstrap::ParamControl> Reflex::Bootstrap::ParamControl::Create(ConstTRef <ParamDesc> param_desc, ConstTRef <Value32> value)
{
	TRef <ParamControl> control = kNoValue;

	switch (param_desc->type)
	{
	case ParamDesc::kTypeBool:
		control = New<ButtonControl>(param_desc, value);
		break;

	case ParamDesc::kTypeEnum:
		if (param_desc)
		{
			control = New<PopupControl>(Cast<EnumParamDesc>(param_desc), value);
		}
		else
		{
			control = New<DummyControl>(param_desc);
		}
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

	GLX::SetEventDelegate(control, {}, [audioplugin = instance.instance, paramidx](GLX::Object & src, GLX::Event & e)
	{
		switch (GLX::GetTransactionStage(e))
		{
		case GLX::kTransactionBegin:
			audioplugin->BeginAutomation(paramidx);
			break;

		case GLX::kTransactionPerform:
			audioplugin->Automate(paramidx, Data::GetFloat32(e, GLX::kvalue));
			break;

		case GLX::kTransactionEnd:
		case GLX::kTransactionCancel:
			audioplugin->EndAutomation(paramidx);
			break;
		}

		return true;
	});

	return control;
}
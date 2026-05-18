#include "../../../../include/reflex_ext/glx/widgets/dragedit.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

constexpr Float kPrecision[4] = { 1.0f, 0.1f, 0.01f, 0.001f };

template <UInt PRECISION> WString Value2Text(Float value)
{
	value = Quantise(value, kPrecision[PRECISION]);

	return ToWString(value, PRECISION);
}

Float Text2Value(const WString & text)
{
	return ToFloat32(ToCString(text));
}

const FunctionPointer <WString(Float)> kValue2Text[4] =
{
	&Value2Text<0>,
	&Value2Text<1>,
	&Value2Text<2>,
	&Value2Text<3>
};

REFLEX_END_INTERNAL

Reflex::GLX::DragEdit::~DragEdit()
{
	ClearDelegate(kZeroKey);	//NEEDED , otherwise functor can be called in base dtor
}

Reflex::GLX::DragEdit::DragEdit()
	: m_value2text(Bind(&Value2Text<0>, _P1)),
	m_text2value(Bind(&Text2Value, _P1)),
	m_sensitivity_factor(2.0f),
	m_text(false)
{
	SetEventDelegate(*this, kZeroKey, [this](Object & src, Event & e)
	{
		if (e.id == RotarySlider::kDoubleClick)
		{
			auto behaviour = TextEditBehaviour::Create();

			SetDelegate(TextEditBehaviour::kTextEdit, behaviour);

			return true;
		}
		else if (e.id == kTransaction)
		{
			if (GetIndex(e) == TextEditBehaviour::kTextEdit)
			{
				auto stage = GetTransactionStage(e);

				if (stage >= kTransactionEnd)
				{
					ClearDelegate(TextEditBehaviour::kTextEdit);

					if (stage != kTransactionCancel)
					{
						SetValueTransacted(m_text2value(m_text.GetView()));
					}
				}

				return true;
			}
			else
			{
				Update();
			}
		}
		else if (e.id == kFocus)
		{
			if (QueryAntecedent(e, kKeyDown))
			{
				auto behaviour = TextEditBehaviour::Create();

				SetDelegate(TextEditBehaviour::kTextEdit, behaviour);
			}
			//need this so that TextEditBehaviour will receive LoseFocusMsg

			return true;
		}

		return false;
	});

	SetProperty(kvalue, m_text);

	Update();
}

void Reflex::GLX::DragEdit::SetTextConverter(const Function <WString(Float)> & value2text, const Function <Float(const WString&)> & text2value)
{
	m_value2text = value2text;

	m_text2value = text2value;

	Update();
}

void Reflex::GLX::DragEdit::SetSensitivity(Float factor)
{
	m_sensitivity_factor = factor;

	auto [range,step] = GetRange();

	SetRange(range.start, range.start + range.length, step);
}

void Reflex::GLX::DragEdit::SetPrecision(UInt precision)
{
	m_value2text = Bind(Copy(kValue2Text[Reflex::Min<UInt>(precision, 3)]), _P1);

	Update();
}

bool Reflex::GLX::DragEdit::SetRange(Float min, Float max, Float step)
{
	RotarySlider::SetSensitivity(Abs(max - min) * m_sensitivity_factor);

	if (RotarySlider::SetRange(min, max, step))
	{
		Update();

		return true;
	}
	else
	{
		return false;
	}
}

bool Reflex::GLX::DragEdit::SetValue(Float value)
{
	if (RotarySlider::SetValue(value))
	{
		Update();

		return true;
	}
	else
	{
		return false;
	}
}

void Reflex::GLX::DragEdit::OnUpdate()
{
	m_text.SetValue(m_value2text(GetValue()));

	Accommodate();
}

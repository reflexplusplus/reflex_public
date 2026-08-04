#include "../../../../include/reflex_ext/glx/widgets/rotaryslider.h"
#include "common.h"



//
//TODO need GenericProperties / WidgetWithProperties




//
//implementation

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

struct RotaryDisplayPropertiesDelegate : public GLX::Object::Delegate
{
	REFLEX_OBJECT(GLX::RotaryDisplayPropertiesDelegate, Delegate);

	RotaryDisplayPropertiesDelegate(Float value_origin, Key32 angle_id, Key32 sweep_id)
		: m_sweep_range(kNormalRange)
		, m_value_origin(value_origin)
		, m_sweep_id(sweep_id)
		, m_angle_id(angle_id)
	{
	}

	void OnAttachObject() override
	{
		m_range = GetProperty<RangeProperty>(object, krange);

		m_value = GetProperty<Data::Float32Property>(object, kvalue);

		m_angle = Data::Detail::AcquireProperty<Data::Float32Property>(object, m_angle_id);

		m_sweep = Data::Detail::AcquireProperty<GLX::RangeProperty>(object, m_sweep_id);

		OnSetStyle(object->GetCurrentStyle());
	}

	void OnSetStyle(const Style & style) override
	{
		auto range = Data::GetFloat32Array(style, K32("sweep_range"));

		if (range.size == 2)
		{
			m_sweep_range = { range.GetFirst(), range.GetLast() };
		}
		else
		{
			m_sweep_range = kNormalRange;
		}

		OnUpdate();
	}

	void OnUpdate() override
	{
		auto [angle, sweep] = CalcRotaryAngleAndSweep(m_range->value, m_value->value, m_value_origin, m_sweep_range);

		m_angle->value = angle;

		m_sweep->value = sweep;

		object->Realign();
	}

	Range m_sweep_range;
	Float m_value_origin;

	Key32 m_sweep_id;
	Key32 m_angle_id;

	ConstReference <RangeProperty> m_range;
	ConstReference <Data::Float32Property> m_value;

	Reference <RangeProperty> m_sweep;
	Reference <Data::Float32Property> m_angle;
};

constexpr UInt32 kIncrementer = K32("{Incrementer}");

REFLEX_END_INTERNAL

Reflex::GLX::RotarySlider::RotarySlider()
	: m_sensitivity(128.0f),
	m_quantise(0.0f),
	m_default(0.0f),
	m_range(New<RangeProperty>(kNormalRange)),
	m_value(New<Data::Float32Property>())
{
	//publish properties once for IDE 

	Data::PropertySet::OnSetProperty(MakeAddress<RangeProperty>(krange), m_range);					

	Data::PropertySet::OnSetProperty(MakeAddress<Data::Float32Property>(kvalue), m_value);


	SetFlow(*this, kFlowY | kFlowInvert);

	EnableMouseCapture(*this);

	SetMouseCursor(kMouseCursorPointer);

	Update();

	EnableOnAttachDetachWindow();
}

void Reflex::GLX::RotarySlider::SetSensitivity(Float pixels)
{
	m_sensitivity = pixels;
}

void Reflex::GLX::RotarySlider::SetDefault(Float value)
{
	m_default = value;
}

bool Reflex::GLX::RotarySlider::SetRange(Float min, Float max, Float step)
{
	m_quantise = step;

	if (SetFiltered<GLX::Range>(m_range->value, { min, max - min }))
	{
		Accommodate();	//for image case

		Update();

		return true;
	}

	return false;
}

void Reflex::GLX::RotarySlider::SetValueTransacted(Float32 value)
{
	if (SetValue(value))
	{
		Core::WeakReference ref = { *this };

		Detail::PerformTransaction(ref, 0);

		Detail::EndTransaction(ref, 0);
	}
}

void Reflex::GLX::RotarySlider::Reset()
{
	SetValueTransacted(m_default);
}

bool Reflex::GLX::RotarySlider::SetValue(Float value)
{
	auto & range = m_range->value;

	if (SetFiltered(m_value->value, ClipValue(value, range)))
	{
		Accommodate();	//for image case

		Update();

		return true;
	}

	return false;
}

Reflex::Float Reflex::GLX::RotarySlider::GetValue() const
{
	return m_value->value;
}

bool Reflex::GLX::RotarySlider::OnEvent(Object & src, Event & e)
{
	REFLEX_ASSERT(&src == this);	//these &src == this should not be needed anymore

	bool yaxis = GetAxis(*this);

	if (e.id == kMouseDown && &src == this)
	{
		auto flags = GetClickFlags(e);

		if (flags & kClickFlagRmb)
		{
			EnableMouseCapture(*this);

			if (!OfferUp(*this, e)) Reset();
		}
		else if (flags & kClickFlagDbl)
		{
			EnableMouseCapture(*this, false);

			if (!GLX::Emit(*this, kDoubleClick)) Reset();
		}
		else
		{
			auto inc = Detail::Incrementer::Create();

			SetProperty(kIncrementer, inc);

			auto & range = m_range->value;

			inc->SetRange(range.start, range.start + range.length, m_quantise, m_sensitivity, yaxis);
			
			inc->Begin(m_value->value);
						
			EnableMouseCapture(*this, true, true);
		}

		return true;
	}
	else if (e.id == kMouseDrag && &src == this)
	{
		if (auto inc = QueryProperty<Detail::Incrementer>(kIncrementer))
		{
			auto delta = GetMouseDelta(e);

			auto value = inc->Process(Detail::GetPoint(yaxis, delta), GetModifierKeys(e) & kModifierKeyShift);

			if (SetValue(value))
			{
				Detail::PerformTransaction(*this, 0);
			}
		}

		return true;
	}
	else if (e.id == kMouseUp && &src == this)
	{
		Detail::EndTransaction(*this, 0);

		UnsetProperty<Detail::Incrementer>(kIncrementer);

		return true;
	}
	else if (e.id == kKeyDown)
	{
		UInt8 modifiers = GetModifierKeys(e);

		UInt8 ignore = modifiers & UInt8(kModifierKeyCtrl | kModifierKeyAlt | kModifierKeySystem);

		if (!ignore && !QueryDelegate(TextEditBehaviour::kDynamicTypeInfo))
		{
			if (auto step = GetRange().b)
			{
				step *= (modifiers & kModifierKeyShift) ? 0.1f : 1.0f;

				auto value = GetValue();
				
				auto trap = false;

				switch (GetKeyCode(e))
				{
				case kKeyCodeDown:
				case kKeyCodeNumericMinus:
					value -= step;
					trap = true;
					break;
					
				case kKeyCodeUp:
				case kKeyCodeNumericPlus:
					value += step;
					trap = true;
					break;
				}
				
				SetValueTransacted(value);
				
				return trap;
			}
		}
	}

	return Object::OnEvent(src, e);
}

void Reflex::GLX::RotarySlider::OnDetachWindow()
{
	Detail::EndTransaction(*this, 0);
}

Reflex::GLX::Trap Reflex::GLX::RotarySlider::OnMouseOver(Core::MouseAction mouseaction, UInt8 flags)
{
	if (Detail::GetBool(GetCurrentStyle(), K32("circular"), True(GetLayoutFlags() & GLX::kFlowY)))
	{
		if (CircleContains(GetRect().size.w, GetMousePosition(*this)))
		{
			return Object::OnMouseOver(mouseaction, flags);
		}
		else
		{
			return kTrapThru;
		}
	}
	else
	{
		return Object::OnMouseOver(mouseaction, flags);
	}
}

void Reflex::GLX::RotarySlider::OnQueryProperty(Address address, Reflex::Object * & pobject) const
{
	if (address == MakeAddress<RangeProperty>(krange))
	{
		pobject = m_range.Adr();
	}
	else if (address == MakeAddress<Data::Float32Property>(kvalue))
	{
		pobject = m_value.Adr();
	}
	else
	{
		return Object::OnQueryProperty(address, pobject);
	}
}

void Reflex::GLX::RotarySlider::OnSetProperty(Address adr, Reflex::Object & object)
{
	if (adr == MakeAddress<Data::Float32Property>(K32("sensitivity")))
	{
		auto t = AutoRelease(object);

		return SetSensitivity(Reflex::Max(Cast<Data::Float32Property>(object)->value, 1.0f));
	}
	else if (adr == MakeAddress<RangeProperty>(krange))
	{
		auto t = AutoRelease(object);

		auto & range = Cast<RangeProperty>(object)->value;

		SetRange(range.start, range.start + range.length, m_quantise);
	}
	else if (adr == MakeAddress<Data::Float32Property>(kvalue))
	{
		auto t = AutoRelease(object);

		SetValue(Cast<Data::Float32Property>(object)->value);
	}
	else
	{
		Object::OnSetProperty(adr, object);
	}
}

Reflex::TRef <Reflex::GLX::Object::Delegate> Reflex::GLX::AttachRotaryDisplayPropertiesDelegate(GLX::Object & object, Float value_origin, Key32 angle_id, Key32 sweep_id)
{
	auto d = New<RotaryDisplayPropertiesDelegate>(value_origin, angle_id, sweep_id);

	object.SetDelegate(angle_id, d);

	return d;
}

Reflex::Tuple <Reflex::Float, Reflex::GLX::Range> Reflex::GLX::CalcRotaryAngleAndSweep(Range value_range, Float value, Float value_origin, Range sweep_range)
{
	if (value_range.length)
	{
		auto inv_len = 1.0f / value_range.length;

		auto  t_value =  Clip((value - value_range.start) * inv_len, 0.0f, 1.0f);

		auto t_origin = Clip((value_origin - value_range.start) * inv_len, 0.0f, 1.0f);

		auto angle = sweep_range.start + (sweep_range.length * t_value);

		Range sweep = { sweep_range.start + (sweep_range.length * t_origin), sweep_range.length * (t_value - t_origin) };

		return { angle, sweep };
	}
	else
	{
		return { sweep_range.start, { sweep_range.start, 0.0f } };
	}
}

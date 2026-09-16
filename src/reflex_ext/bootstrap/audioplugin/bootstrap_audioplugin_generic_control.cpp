#include "../../../../include/reflex_ext/bootstrap/audioplugin/ui/generic_control.h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap)

constexpr Key32 kControlTypeStates[GenericControl::kNumType] =
{
	K32("continuous"),
	K32("discrete"),
	K32("enumeration"),
	K32("boolean"),
};

struct ControlCompiledStyle : public Object
{
	ControlCompiledStyle(const GLX::Style & style)
		: content_style(style.QuerySubStyle(GLX::kcontent, &GLX::Style::null))
		, positioning_flags(ParseControlPositioning(style))
		, flow_flags(ParseControlFlow(style))
	{
		if (auto labels = style.QuerySubStyle("labels"))
		{
			auto labels_itr = labels->Iterate<Data::CStringProperty>();

			remap_store.Allocate(labels_itr.GetSize());

			for (auto & item : labels_itr)
			{
				auto & label = remap_store.Push<kAllocateNone>(ToWString(item.value->value));

				remap_view.Set(item.key.id, label);
			}
		}
	}

	static UInt8 ParseControlPositioning(const GLX::Style & style)
	{
		Pair <GLX::Orientation> alignment = { GLX::kOrientationCenter, GLX::kOrientationNear };

		constexpr Key32 kAlignmentProperties[] = { K32("align_content"), GLX::kalign };

		for (auto id : kAlignmentProperties)
		{
			if (auto value = style.QueryProperty<Data::Key32Property>(id))
			{
				alignment = GLX::Detail::kAlignmentToOrientation[GLX::Detail::ParseAlignment(value->value, GLX::kAlignmentTop)];
			}
			else if (auto values = Data::GetKey32Array(style, id))
			{
				if (values.size > 1)
				{
					alignment = { GLX::Detail::ParseOrientation(values.GetFirst(), GLX::kOrientationFit), GLX::Detail::ParseOrientation(values.GetLast(), GLX::kOrientationFit) };
				}
				else
				{
					alignment = GLX::Detail::kAlignmentToOrientation[GLX::Detail::ParseAlignment(values.GetFirst(), GLX::kAlignmentTop)];
				}
			}
			else continue;

			REFLEX_IF_DEBUG(if (id == GLX::kalign) Reflex::GLX::output.Warn("GenericControl style property 'align' is deprecated; use 'align_content'"));

			break;
		}

		return UInt8(GLX::Detail::kPositioningFloat) | (UInt8(alignment.a) << 2) | (UInt8(alignment.b) << 4);
	}

	static UInt8 ParseControlFlow(const GLX::Style & style)
	{
		bool y = Data::GetKey32(style, K32("axis"), K32("y")) == K32("y");

		bool invert = Data::GetBool(style, K32("invert"), y);

		return UInt8((y ? GLX::kFlowY : GLX::kFlowX) | (invert ? GLX::kFlowInvert : GLX::kFlowX));
	}


	Array <WString> remap_store;

	Map <Key32, WString::View> remap_view;

	ConstAlreadyRetained <GLX::Style> content_style;

	UInt8 positioning_flags;

	UInt8 flow_flags;
};

REFLEX_END_INTERNAL

Reflex::Bootstrap::GenericControl::GenericControl()
	: m_cstyle(GLX::Detail::Compile<ControlCompiledStyle>(GLX::Style::null))
	, m_type_active({ kNumType, false })
	, m_value(kNewObject)
{
	GLX::SetFlow(*this, GLX::kFlowY);

	SetProperty(GLX::kvalue, m_value);
}

void Reflex::Bootstrap::GenericControl::SetLabel(const WString::View & label)
{
	GLX::SetText(*this, *Cast<ControlCompiledStyle>(m_cstyle)->remap_view.Search(label, &label), K32("label"));

	Accommodate();
}

void Reflex::Bootstrap::GenericControl::SetValueText(const WString::View & value)
{
	m_value->SetValue(value);
}

void Reflex::Bootstrap::GenericControl::ClearWidget()
{
	if (m_type_active.a != kNumType)
	{
		UnsetState(kControlTypeStates[m_type_active.a]);
	}

	m_type_active = { kNumType, false };

	if (m_content)
	{
		m_content->Detach();

		m_content = {};
	}
}

Reflex::AlreadyRetained <Reflex::GLX::Object> Reflex::Bootstrap::GenericControl::AcquireWidget(Type type, bool active)
{
	auto previous_type = m_type_active.a;

	if (SetFiltered(m_type_active, MakeTuple(type, active)))
	{
		m_content->Detach();

		m_content = {};

		if (previous_type != kNumType) UnsetState(kControlTypeStates[previous_type]);

		switch (type)
		{
		case kTypeContinuous:
			m_content = REFLEX_CREATE(GLX::RotarySlider);
			GLX::AttachRotaryDisplayPropertiesDelegate(m_content);
			break;

		case kTypeDiscrete:
			m_content = REFLEX_CREATE(GLX::DragEdit);
			break;

		case kTypeEnumeration:
			m_content = REFLEX_CREATE(GLX::Popup);
			break;

		case kTypeBoolean:
			m_content = REFLEX_CREATE(GLX::Button);
			break;

		default:
			return {};
		}

		SetState(kControlTypeStates[type]);

		m_content->SetProperty(GLX::kvalue, m_value);

		m_content->SetParent(*this);

		GLX::Activate(m_content, active);
	}

	return m_content;
}

void Reflex::Bootstrap::GenericControl::OnSetStyle(const GLX::Style & style)
{
	auto compiled_style = GLX::Detail::Compile<ControlCompiledStyle>(style);

	m_cstyle = compiled_style;

	m_content->SetStyle(compiled_style->content_style);

	m_content->SetPositioningFlags(compiled_style->positioning_flags);

	GLX::SetFlow(m_content, compiled_style->flow_flags);
}

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

void ParseControlAlignment(const GLX::Style & style, Pair <GLX::Orientation> & alignment, const Pair <GLX::Orientation> & fallback)
{
	if (auto value = style.QueryProperty<Data::Key32Property>(GLX::kalign))
	{
		alignment = GLX::Detail::kAlignmentToOrientation[GLX::Detail::ParseAlignment(value->value, GLX::kAlignmentTop)];
	}
	else if (auto values = Data::GetKey32Array(style, GLX::kalign))
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
	else
	{
		alignment = fallback;
	}
}

REFLEX_END_INTERNAL

struct Reflex::Bootstrap::GenericControl::CStyle : public Reflex::Object
{
	CStyle(const GLX::Style & style)
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

	Array <WString> remap_store;

	Map <Key32,WString::View> remap_view;
};

Reflex::Bootstrap::GenericControl::GenericControl()
	: m_cstyle(GLX::Detail::Compile<CStyle>(GLX::Style::null))
	, m_type_active({ kNumType, false })
	, m_value(kNewObject)
{
	GLX::SetFlow(*this, GLX::kFlowY);

	SetProperty(GLX::kvalue, m_value);
}

Reflex::Bootstrap::GenericControl::~GenericControl()
{
}

void Reflex::Bootstrap::GenericControl::SetLabel(const WString::View & label)
{
	GLX::SetText(*this, *m_cstyle->remap_view.Search(label, &label), K32("label"));

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

Reflex::TRef <Reflex::GLX::Object> Reflex::Bootstrap::GenericControl::AcquireWidget(Type type, bool active)
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

		case kTypeBoolean:
			m_content = REFLEX_CREATE(GLX::Button);
			break;

		case kTypeEnumeration:
			m_content = REFLEX_CREATE(GLX::Popup);
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
	m_cstyle = GLX::Detail::Compile<CStyle>(style);

	//if (!m_content) return;

	// Accept the old direct child schema while stylesheets migrate to
	// @State <type> { content: ...; }.
	//if (!content_style && m_type_active.a != kNumType)
	//{
	//	if (auto type_style = style.QuerySubStyle(kControlTypeStates[m_type_active.a]))
	//	{
	//		content_style = type_style->QuerySubStyle(GLX::kcontent, type_style);
	//	}
	//}

	if (auto content_style = style.QuerySubStyle(GLX::kcontent))
	{
		m_content->SetStyle(*content_style);

		Pair <GLX::Orientation> alignment;

		ParseControlAlignment(style, alignment, { GLX::kOrientationCenter, GLX::kOrientationNear });

		GLX::EnableFloat(m_content, alignment.a, alignment.b);
	}

	bool y = Data::GetKey32(style, K32("axis"), K32("y")) == K32("y");

	bool invert = Data::GetBool(style, K32("invert"), y);

	GLX::SetFlow(m_content, GLX::FlowFlags((y ? GLX::kFlowY : GLX::kFlowX) | (invert ? GLX::kFlowInvert : GLX::kFlowX)));
}

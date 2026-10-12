#include "widgets.h"




//
// RotarySlider demo

namespace Widgets { namespace {

struct RotarySliderDemo : public GLX::Object
{
	RotarySliderDemo()
	{
		GLX::SetFlow(*this, GLX::kFlowY | GLX::kFlowCenter);

		auto & rotary = m_rotaries[0];
		rotary.SetRange(-1.0f, 1.0f, 0.0f);
		rotary.SetDefault(0.0f);
		rotary.SetValue(0.0f);
		GLX::AttachRotaryDisplayPropertiesDelegate(rotary);

		auto & bitmap_rotary = m_rotaries[1];
		bitmap_rotary.SetRange(0.0f, 1.0f, 0.0f);
		bitmap_rotary.SetDefault(0.5f);
		bitmap_rotary.SetValue(0.5f);

		auto & unipolar_slider = m_rotaries[2];
		GLX::SetFlow(unipolar_slider, GLX::kFlowY | GLX::kFlowInvert);
		unipolar_slider.SetRange(0.0f, 1.0f, 0.0f);
		unipolar_slider.SetDefault(0.5f);
		unipolar_slider.SetValue(0.5f);
		GLX::AttachSliderDisplayPropertiesDelegate(unipolar_slider);

		auto & bipolar_slider = m_rotaries[3];
		GLX::SetFlow(bipolar_slider, GLX::kFlowX);
		bipolar_slider.SetRange(-1.0f, 1.0f, 0.0f);
		bipolar_slider.SetDefault(0.0f);
		bipolar_slider.SetValue(0.0f);
		GLX::AttachSliderDisplayPropertiesDelegate(bipolar_slider);
			
		for (auto & i : m_rotaries)
		{
			GLX::AddInline(*this, i, GLX::kOrientationCenter);
		}
	}

	bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (e.id == GLX::kTransaction && DynamicCast<GLX::RotarySlider>(src))
		{
			auto rotary = Cast<GLX::RotarySlider>(src);
			
			output.Log("rotary stage=", UInt(GLX::GetTransactionStage(e)), " value=", rotary->GetValue());
		
			return true;
		}

		return Object::OnEvent(src, e);
	}

	void OnSetStyle(const GLX::Style & style) override 
	{ 
		m_rotaries[0].SetStyle(style["Rotary"]);
		m_rotaries[1].SetStyle(style["BitmapRotary"]);
		m_rotaries[2].SetStyle(style["UnipolarSlider"]);
		m_rotaries[3].SetStyle(style["BipolarSlider"]);
	}

	GLX::RotarySlider m_rotaries[4];
};

} }

const Widgets::DemoDesc Widgets::g_rotary_slider = Widgets::MakeDemo("rotary_slider", L"RotarySlider", false, []() -> Unretained <GLX::Object>
{
	return New<RotarySliderDemo>();
});

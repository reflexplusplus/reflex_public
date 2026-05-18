#include "reflex_ext/bootstrap/common/functions.h"




//
//impl

REFLEX_NS(Reflex::System::Common)

extern UInt GetValue(const Renderer::Config & config, Key32 key, UInt32 fallback);

REFLEX_END

REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap)

struct GraphicsSettings : public GLX::Object
{
	GraphicsSettings(const GLX::Style & styles, bool resizeable);


	virtual bool OnEvent(GLX::Object & source, GLX::Event & e) override;

	virtual void OnUpdate() override;


	void ToggleOption(Key32 option);

	static void SetGraphicsConfig(const System::Renderer::Config & config, bool restart = true)
	{
		global->prefs->SetProperty(Detail::kViewGraphicsConfig, REFLEX_CREATE(Data::MapOfKey32Property<UInt>, config));

		if (restart) GLX::Restart();
	}


	Array < Tuple < Key32, UInt, WString::View> > m_optionids;

	CString m_renderer_name;


	GLX::Form m_glx;

	GLX::Form m_system;

	GLX::Form m_renderer;


	Array <TRef <GLX::Form>> m_options;


	static constexpr const WChar * kDpiAware = L"DPI Aware";
	static constexpr const WChar * kRetina = L"Retina";
	static constexpr const WChar * kNativePixelDensity = L"Native Pixel Density";

	static constexpr WString::View kHdLabels[System::kNumPlatform] = { kDpiAware, kRetina, kNativePixelDensity, kNativePixelDensity, kRetina, kNativePixelDensity };
};

GraphicsSettings::GraphicsSettings(const GLX::Style & styles, bool resizeable)
	: m_glx(L"GLX"),
	m_system(L"System"),
	m_renderer(L"Renderer")
{
	GLX::SetFlow(*this, GLX::kFlowY);

	GLX::SetFlow(m_glx.body, GLX::kFlowY);

	GLX::SetFlow(m_system.body, GLX::kFlowY);

	GLX::SetFlow(m_renderer.body, GLX::kFlowY);


	auto section_style = styles[K32("Section")];

	auto popup_style = styles[K32("PopupProperty")];

	auto button_style = styles[K32("ButtonProperty")];

	m_glx.SetStyle(section_style);

	m_system.SetStyle(section_style);


	m_optionids.Push({ GLX::kEmulateMobile, true, L"Emulate Mobile" });

	m_optionids.Push({ GLX::Core::kIncrementalMouse, true, L"Relative Mouse" });

	m_optionids.Push({ GLX::kAnimations, true, L"Animated Transitions" });


	m_optionids.Push({ System::Renderer::kHD, 0, kHdLabels[System::kPlatform] });

	//m_optionids.Push({ System::Renderer::kAA, 0, L"Anti-Aliasing" });

	m_optionids.Push({ System::Renderer::kTX, 0, L"Texture Rendering" });


	auto & config = GLX::Core::desktop->GetGraphicsConfig();

	auto engines = System::Renderer::GetEngines();

	if (engines.GetSize() > 1)
	{
		GLX::AddInline(m_system.body, m_renderer);

		InitPopup(m_renderer, popup_style);
	}


	TRef <GLX::Object> parents[] = { m_system.body, m_glx.body };

	for (auto & i : m_optionids)
	{
		if (i.b || config.Search(i.a))
		{
			auto button = GLX::AddInline(parents[i.b], New<GLX::Form>(i.c));

			m_options.Push(InitButton(*button, button_style, i.a));
		}
	}

	GLX::AddInline(*this, m_glx);

	GLX::AddInline(*this, m_system);

	Update();
}

bool GraphicsSettings::OnEvent(GLX::Object & src, GLX::Event & e)
{
	if (auto menu = GLX::GetMenu(e); menu && src == m_renderer.body)
	{
		for (auto & i : System::Renderer::GetEngines())
		{
			GLX::BindClick(GLX::AddMenuOption(menu, ToWString(i.a), m_renderer_name == i.a), [i]()
			{
				auto config = GLX::Core::desktop->GetGraphicsConfig();

				config.Set(GLX::Core::kEngine, Key32(i.a).value);

				SetGraphicsConfig(config);
			});
		}

		return true;
	}
	else if (e.id == GLX::kMouseDown)
	{
		if (IsValidKey(src.id))
		{
			ToggleOption(src.id);

			return true;
		}
	}

	return false;
}

void GraphicsSettings::OnUpdate()
{
	m_renderer_name = GLX::Core::g_renderer->GetEngineName();

	GLX::SetText(m_renderer.body, ToWString(m_renderer_name));


	auto & config = GLX::Core::desktop->GetGraphicsConfig();

	for (auto & i : m_options)
	{
		auto button = i->body;

		if (auto ptr = config.Search(button->id))
		{
			GLX::Select(button, True(*ptr));
		}
		else
		{
			GLX::Select(button, false);
		}
	}
}

void GraphicsSettings::ToggleOption(Key32 id)
{
	auto config = GLX::Core::desktop->GetGraphicsConfig();

	auto enabled = True(System::Common::GetValue(config, id, false));

	enabled = !enabled;

	config.Set(id, enabled);

	if (id == System::Renderer::kHD && System::kPlatform == System::kPlatformWindows)
	{
		GLX::output.Log(L"DPI Awareness will be", enabled ? L"enabled" : L"disabled", L"on next start");

		SetGraphicsConfig(config, false);

		Update();
	}
	else
	{
		SetGraphicsConfig(config);
	}
}

REFLEX_END_INTERNAL

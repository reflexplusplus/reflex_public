#include "settingsdialog.h"

#include "settingsdialog/graphics.cpp"

#if defined (REFLEX_BOOTSTRAP_TYPE_AUDIOAPP) || defined (REFLEX_BOOTSTRAP_TYPE_AUDIOPLUGIN)
#include "settingsdialog/audiomidi.cpp"
#endif




//
//Internal

REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap)

Console::Console()
	: IDE::Detail::ConsolePanel("Bootstrap", 1),
	m_stylesheet(IDE::Detail::RetrieveStyleSheet()),
	m_clear(L"Clear Preferences")
{
	m_tabgroup.SetStyle(m_stylesheet["ConsoleGroup"]);

#if defined (REFLEX_BOOTSTRAP_TYPE_AUDIOAPP) || defined (REFLEX_BOOTSTRAP_TYPE_AUDIOPLUGIN)
	auto GetAudioApp = []() -> System::AudioPlugin *
	{
		if (System::kEnvironmentType != System::kEnvironmentTypeAudioPlugin)
		{
			if (auto papp = System::App::list.GetFirst())
			{
				return DynamicCast<System::AudioPlugin>(*papp);
			}
		}

		return nullptr;
	};

	if (auto audio_app = GetAudioApp())
	{
		m_tabgroup.AddPanel(L"Audio", REFLEX_CREATE(AudioSettings, *audio_app, m_stylesheet));

		m_tabgroup.AddPanel(L"MIDI", REFLEX_CREATE(MidiSettings, *audio_app, m_stylesheet));
	}
#endif

	m_tabgroup.AddPanel(L"Graphics", REFLEX_CREATE(GraphicsSettings, m_stylesheet, true));

	GLX::BindClick(m_clear, []()
	{
		global->EnableIde(false);

		global->prefs->Reset();

		global->CommitPreferences();

		GLX::Restart();
	});

	GLX::SetFlow(*this, GLX::kFlowY);

	auto bar = m_stylesheet["Bar"];

	GLX::AddInlineFlex(*this, m_tabgroup);

	GLX::AddInline(m_footer, GLX::Init(m_clear, bar["Button"]));

	GLX::AddInline(*this, GLX::Init(m_footer, bar));
}

void Console::OnReset(Key32 context)
{
	m_tabgroup.GetSelector()->SelectPanel(0);
}

void Console::OnRestore(Data::Archive::View & stream, Key32 context)
{
	UInt8 idx = Data::Deserialize<UInt8>(stream);

	m_tabgroup.GetSelector()->SelectPanel(idx);
}

void Console::OnStore(Data::Archive & stream) const
{
	Data::Serialize(stream, UInt8(m_tabgroup.GetSelector()->GetCurrentIndex().value));
}

IDE::Detail::ConsolePanel::Ctr gConsolePanelCtr(L"Settings", 1, []() -> TRef <IDE::Detail::ConsolePanel>
{
	return REFLEX_CREATE(Reflex::Bootstrap::Console);
});

REFLEX_END_INTERNAL

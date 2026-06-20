#include "reflex_ext/bootstrap/audioplugin/audioplugin.h"




REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap)

struct AudioMidiSettings : public GLX::Object
{
	AudioMidiSettings(System::AudioPlugin & audioplugin, const GLX::Style & styles);

	virtual bool OnEvent(GLX::Object & src, GLX::Event & e) override;

	virtual void OnUpdate() override;


	void Build(decltype (&System::AudioPlugin::GetAudioChannels) GetChannels, decltype (&System::AudioPlugin::Lock::EnableAudioChannel) EnableChannel)
	{
		AudioMidiSettings::GetChannels = GetChannels;

		AudioMidiSettings::EnableChannel = EnableChannel;

		constexpr WString::View io[2] = { L"In", L"Out" };

		REFLEX_LOOP(idx, 2)
		{
			auto & row = m_bars[idx];

			auto & list = m_lists[idx];

			list.GetContent()->SetSelectionMode(GLX::List::kSelectionModeMultiToggle);

			GLX::AddInline(*this, GLX::Init(row, m_section_style["header"], io[idx]));

			GLX::AddInlineFlex(*this, list);
		}
	}



	const TRef <System::AudioPlugin> audioplugin;

	decltype (&System::AudioPlugin::GetAudioChannels) GetChannels;

	decltype (&System::AudioPlugin::Lock::EnableAudioChannel) EnableChannel;

	Reference <Reflex::Object> m_listener;

	ConstTRef <GLX::Style> m_section_style;

	ConstTRef <GLX::Style> m_popup_style;

	ConstTRef <GLX::Style> m_listitem_style;

	GLX::Object m_bars[2];

	GLX::ListScroller m_lists[2];
};

struct AudioSettings : public AudioMidiSettings
{
	AudioSettings(System::AudioPlugin & audioplugin, const GLX::Style & styles)
		: AudioMidiSettings(audioplugin, styles),
		m_device(L"Device"),
		m_soundcard(L"SoundCard"),
		m_samplerate(L"SampleRate"),
		m_latency(L"Latency"),
		m_showpanel(L"Control Panel")
	{
		GLX::SetFlow(m_device.body, GLX::kFlowY);

		m_device.SetStyle(m_section_style);

		GLX::AddInline(m_device.body, InitPopup(m_soundcard, m_popup_style));

		GLX::AddInline(m_device.body, InitPopup(m_samplerate, m_popup_style));

		GLX::AddInline(m_device.body, InitPopup(m_latency, m_popup_style));

		InitButton(m_showpanel, styles["ButtonProperty"], kNullKey);

		GLX::AddInline(*this, m_device);

		Build(&System::AudioPlugin::GetAudioChannels, &System::AudioPlugin::Lock::EnableAudioChannel);
	}

	virtual bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (auto menu = GLX::GetMenu(e))
		{
			menu->Clear();

			if (src == m_soundcard.body)
			{
				for (auto & i : audioplugin->GetAvailableAudioDevices())
				{
					GLX::BindClick(menu->AddItem(i), [this, i]()
					{ 
						System::AudioPlugin::Lock lock(audioplugin);

						Detail::SelectAudioDevice(lock, i);
						
						Update(); 
					});
				}
			}
			else if (src == m_samplerate.body)
			{
				for (auto & i : audioplugin->GetAvailableSampleRates())
				{
					GLX::BindClick(menu->AddItem(ToWString(i, 0)), [this, i]() 
					{
						System::AudioPlugin::Lock lock(audioplugin);
						
						lock.RequestSampleRate(i); 
						
						Update(); 
					});
				}
			}
			else if (src == m_latency.body)
			{
				for (auto & i : audioplugin->GetAvailableBufferSizes())
				{
					GLX::BindClick(menu->AddItem(ToWString(i)), [this, i]() 
					{ 
						System::AudioPlugin::Lock lock(audioplugin);

						lock.RequestBufferSize(i);
						
						Update(); 
					});
				}
			}

			return true;
		}
		else if (e.id == GLX::kMouseDown && src == m_showpanel.body)
		{
			Data::SetBool(audioplugin, "ShowControlPanel", true);

			return true;
		}

		return AudioMidiSettings::OnEvent(src, e);
	}

	virtual void OnUpdate() override
	{
		WString device_name = audioplugin->GetCurrentAudioDevice();

		GLX::SetText(m_soundcard.body, device_name ? ToView(device_name) : ToView(L"No Device"));

		GLX::SetText(m_samplerate.body, ToWString(audioplugin->GetCurrentSampleRate(), 0));

		GLX::SetText(m_latency.body, ToWString(audioplugin->GetCurrentBufferSize()));

		if (GetProperty<Data::BoolProperty>(audioplugin, "HasControlPanel")->value)
		{
			GLX::AddInline(m_device.body, m_showpanel);
		}
		else
		{
			m_showpanel.Detach();
		}

		AudioMidiSettings::OnUpdate();
	}


	GLX::Form m_device;

	GLX::Form m_soundcard, m_samplerate, m_latency, m_showpanel;
};

struct MidiSettings : public AudioMidiSettings
{
	MidiSettings(System::AudioPlugin & audioplugin, const GLX::Style & styles)
		: AudioMidiSettings(audioplugin, styles)
	{
		Build(&System::AudioPlugin::GetMidiPorts, &System::AudioPlugin::Lock::EnableMidiPort);
	}
};

AudioMidiSettings::AudioMidiSettings(System::AudioPlugin & audioplugin, const GLX::Style & styles)
	: audioplugin(audioplugin),
	m_listener(System::CreateListener(System::kNotificationChangeDevices, this, [](void * self) 
	{ 
		Cast<GLX::Object>(self)->Update(); 
	})),
	m_section_style(styles["Section"]),
	m_popup_style(styles["PopupProperty"]),
	m_listitem_style(styles["InfoItem"])
{
	SetStyle(m_section_style["body"]);

	GLX::SetFlow(*this, GLX::kFlowY);

	Update();
}

bool AudioMidiSettings::OnEvent(GLX::Object & src, GLX::Event & e)
{
	if (e.id == GLX::List::kListSelect)
	{
		System::AudioPlugin::Lock lock(audioplugin);

		REFLEX_LOOP(output, 2)
		{
			auto & list = m_lists[output];

			if (&src == list.GetContent().Adr())
			{
				(lock.*EnableChannel)(True(output), GLX::GetIndex(e), Data::GetBool(e, GLX::kstate));

				Update();

				return true;
			}
		}
	}

	return false;
}

void AudioMidiSettings::OnUpdate()
{
	REFLEX_LOOP(output, 2)
	{
		auto list = m_lists[output].GetContent();

		auto channels = ((*audioplugin).*GetChannels)(True(output));

		list->Clear();

		UInt idx = 0;

		for (auto & i : channels)
		{
			GLX::Select(GLX::AddInline(list, GLX::Init(IDE::Detail::CreateInfoItem(ToWString(++idx), i.a, false), m_listitem_style)), i.b);
		}
	}
}

REFLEX_END_INTERNAL

#include "plugin.h"




//
//implementation

Reflex::System::Common::PluginInstance::PluginInstance(PluginSession & session, const Configuration::Class & cls)
	: session(session)
	, cls(cls)
	, m_buffersize(512)
	, m_samplerate(1)
	, m_process_rt([](Callbacks&, UInt) {})
	, m_hostwindow(nullptr)
	, m_host_resizing(0)
	, m_resizable_window(false)
{
	Attach(System::App::list);

	m_audiochannels[0].SetSize(cls.channels_io.a);

	m_audiochannels[1].SetSize(cls.channels_io.b);
}

void Reflex::System::Common::PluginInstance::Initialise()
{
	auto & config = session->config;

	auto client = config.instance_ctr(session->client, cls, *this);

	m_callbacks = GetInterface<Callbacks>(client);

	m_client = client;
}

void Reflex::System::Common::PluginInstance::Deinitialise()
{
	m_callbacks = {};

	m_client.Clear();
}

void Reflex::System::Common::PluginInstance::MakeAudioPinName(bool output, bool stereo, UInt pin_index, char * out_buffer, UInt out_buffer_capacity)
{
	if (stereo)
	{
		UInt first = (pin_index * 2) + 1;

		auto name = Join(kInOut[output], ' ', ToCString(first), '+', ToCString(first + 1));

		RawStringCopy(name.GetData(), out_buffer, out_buffer_capacity);
	}
	else
	{
		auto name = Join(kInOut[output], ' ', ToCString(pin_index + 1));
		
		RawStringCopy(name.GetData(), out_buffer, out_buffer_capacity);
	}
}

void Reflex::System::Common::PluginInstance::OpenEditor()
{
	if (m_hostwindow)
	{
		ShowEditor(m_hostwindow);
	}
	else
	{
		CloseEditor();
	}
}

void Reflex::System::Common::PluginInstance::CloseEditor()
{
	m_editor.Clear();
}

Reflex::Array < Reflex::Pair <Reflex::WString, bool> > Reflex::System::Common::PluginInstance::GetAudioChannels(bool output) const
{
	Array < Pair <WString,bool> > rtn;

	rtn.SetSize(output ? cls.channels_io.b : cls.channels_io.a);

	rtn.Fill({ {}, true });

	return rtn;
}

const Reflex::System::EnvironmentType Reflex::System::kEnvironmentType = Reflex::System::kEnvironmentTypeAudioPlugin;

void Reflex::System::App::Quit()
{
}

Reflex::WString Reflex::System::AudioPlugin::GetDefaultDevice(bool recording)
{
	return {};
}

Reflex::System::AudioPlugin::Lock::Lock(AudioPlugin & audioplugin)
	: audioplugin(audioplugin)
{
}

Reflex::System::AudioPlugin::Lock::~Lock()
{
}

bool Reflex::System::AudioPlugin::Lock::SelectAudioDevice(const WString::View & device)
{
	return false;
}

void Reflex::System::AudioPlugin::Lock::RequestSampleRate(Float sr)
{
}

void Reflex::System::AudioPlugin::Lock::RequestBufferSize(UInt buffersize)
{
}

void Reflex::System::AudioPlugin::Lock::EnableAudioChannel(bool output, UInt32 idx, bool enable)
{
}

void Reflex::System::AudioPlugin::Lock::EnableMidiPort(bool output, UInt32 idx, bool enable)
{
}

#include "audioapp.h"
#include "instance.cpp"




//
//impl

void Reflex::System::Common::AudioAppBase::ClearBuffers()
{
	m_audiochannels[0].Clear();

	m_audiochannels[1].Clear();

	m_updated = true;
}

void Reflex::System::Common::AudioAppBase::AddBuffer(bool output, Float * channel)
{
	m_audiochannels[output].Push(channel);

	m_updated = true;
}

bool Reflex::System::Common::AudioAppBase::PrepareProcessing(UInt32 buffersize, Float32 samplerate)
{
	if (Or(SetFiltered(m_config_z, MakeTuple(buffersize, samplerate)), SetFiltered(m_updated, false)))
	{
		ArrayView <const Float*> audioin = { m_audiochannels[0].GetData(), m_audiochannels[0].GetSize() };

		m_process_rt = m_callbacks->OnPrepare(buffersize, samplerate, m_eventsin, m_eventsout, audioin, m_audiochannels[1]);

		return true;
	}

	return false;
}

void Reflex::System::Common::AudioAppBase::OnStartAudio(Object & client)
{
	REFLEX_ASSERT(!m_audio_started);

	m_callbacks = GetInterface<Callbacks>(client);

	Resume();

	m_audio_started = true;
}

void Reflex::System::Common::AudioAppBase::OnStopAudio()
{
	REFLEX_ASSERT(m_audio_started);

	Pause();

	m_audio_started = false;
}

void Reflex::System::Common::AudioAppBase::Pause()
{
	REFLEX_ASSERT_MAINTHREAD("System::Common::AudioAppBase::Pause");

	if (REFLEX_ATOMIC_INC(m_lockcount_fg, 1) == 0)
	{
		Common::output.Log("Common::AudioAppBase OnPause");

		OnPause();	//ask the implementation to stop the audio driver, which should block until finished

		while (REFLEX_ATOMIC_READ(m_isprocessing_rt))	//wait for current slice to end	(shouldnt ever happen if OnPause is properly blocking)
		{
			REFLEX_ASSERT(false);

			System::SuspendThread(10);
		}
	}
}

void Reflex::System::Common::AudioAppBase::Resume()
{
	REFLEX_ASSERT(m_lockcount_fg);

	if (REFLEX_ATOMIC_DEC(m_lockcount_fg, 1) == 1)
	{
		Common::output.Log("Common::AudioAppBase OnResume");

		OnResume();
	}
}

Reflex::System::AudioPlugin::Lock::Lock(AudioPlugin & audioplugin)
	: audioplugin(audioplugin)
{
	Cast<Common::AudioAppBase>(audioplugin)->Pause();
}

Reflex::System::AudioPlugin::Lock::~Lock()
{
	Cast<Common::AudioAppBase>(audioplugin)->Resume();
}

bool Reflex::System::AudioPlugin::Lock::SelectAudioDevice(const WString::View & device)
{
	return Cast<Common::AudioAppBase>(audioplugin)->OnSelectAudioDevice(device);
}

void Reflex::System::AudioPlugin::Lock::RequestSampleRate(Float sr)
{
	Cast<Common::AudioAppBase>(audioplugin)->OnRequestSampleRate(sr);
}

void Reflex::System::AudioPlugin::Lock::RequestBufferSize(UInt buffersize)
{
	Cast<Common::AudioAppBase>(audioplugin)->OnRequestBufferSize(buffersize);
}

void Reflex::System::AudioPlugin::Lock::EnableAudioChannel(bool output, UInt32 idx, bool enable)
{
	Cast<Common::AudioAppBase>(audioplugin)->OnEnableAudioChannel(output, idx, enable);
}

void Reflex::System::AudioPlugin::Lock::EnableMidiPort(bool output, UInt32 idx, bool enable)
{
	Cast<Common::AudioAppBase>(audioplugin)->OnEnableMidiPort(output, idx, enable);
}

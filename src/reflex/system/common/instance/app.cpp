#include "standalone.h"
#include "instance.cpp"




//
//Standalone application without audio

Reflex::System::AudioPlugin::Lock::Lock(AudioPlugin & audioplugin)
	: audioplugin(audioplugin)
{
}

Reflex::System::AudioPlugin::Lock::~Lock()
{
}

Reflex::WString Reflex::System::AudioPlugin::GetDefaultDevice(bool)
{
	return {};
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

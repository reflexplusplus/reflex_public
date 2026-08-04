#include "asio.h"
#include "../library.h"
#include "../../common/simd_utils.h"
#include "../../common/com.h"

#include <audioclient.h>
#include <mmdeviceapi.h>
#include <avrt.h>
#include <Functiondiscoverykeys_devpkey.h>
#pragma comment(lib, "avrt.lib")




#define WASAPI_CHECK(hr, msg) if (FAILED(hr)) throw(CString::View(msg));

REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

struct WASAPI : public Common::DesktopAudioAppBase
{
	//lifetime

	WASAPI();

	~WASAPI();


	typedef Array < Pair <WString, HANDLE> > MidiPortList;

	enum AudioFormat
	{
		kAudioFormatUnknown,
		kAudioFormatInt16,
		kAudioFormatInt24,
		kAudioFormatInt32,
		kAudioFormatFloat32,
		kAudioFormatFloat64,

		kNumAudioFormat,
	};

	enum State
	{
		kStateDriver,
		kStateBuffers,
		kStateStart,
	};

	typedef Pair <AudioFormat, CString::View> AsioFormat;


	virtual void OnPause();

	virtual void OnResume();


	virtual Array < Pair <WString,bool> > GetMidiPorts(bool output) const override;

	virtual void OnEnableMidiPort(bool output, UInt idx, bool enable) override;


	virtual bool OnSelectAudioDevice(const WString::View & value) override;

	virtual WString GetCurrentAudioDevice() const override { return m_current_device; }

	virtual Array <WString> GetAvailableAudioDevices() const override;


	virtual void OnRequestBufferSize(UInt32 value) override;

	virtual void OnRequestSampleRate(Float32 value) override;

	virtual UInt32 GetCurrentBufferSize() const override;

	virtual Float32 GetCurrentSampleRate() const override;


	virtual Array <UInt32> GetAvailableBufferSizes() const override { return Mid<true>(kStandardBufferSizes, 4, kMaxUInt32); }

	virtual Array <Float32> GetAvailableSampleRates() const override;



	virtual Array < Pair <WString,bool> > GetAudioChannels(bool output) const override;

	virtual void OnEnableAudioChannel(bool output, UInt32 idx, bool enable) override;



	//internal

	void EnumerateDevices(const Function <void(IMMDevice * device)> & callback) const;

	WString GetDeviceName(IMMDevice * device) const;


	template <bool OUTPUT> static HANDLE OpenMidiPort(WASAPI & self, UInt sytemidx);

	template <bool OUTPUT> static void CloseMidiPort(WASAPI & self, HANDLE & handle);


	void OnError(const CString::View & error);

	void CloseDriver();



	//callbacks

	static void CALLBACK MidiInProc(HMIDIIN, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);



	WString m_current_device;
	
	Common::ComPtr <IMMDevice> m_device;

	Common::ComPtr <IAudioClient> m_audio_client;
	
	Common::ComPtr <IAudioRenderClient> m_render_client;

	HANDLE m_event_handle;


	UInt32 m_buffersize;

	Float32 m_samplerate;

	UInt m_nchannel = 0;

	Array <Float> m_buffer;


	std::atomic <bool> m_run;

	Reference <Thread> m_thread;


	inline static WASAPI * st_self = 0;
};

WASAPI::WASAPI()
	: m_event_handle(0)
	, m_buffersize(1024)
	, m_samplerate(44100.0f)
	, m_run(false)
{
}

WASAPI::~WASAPI()
{
	////stop midi

	//REFLEX_LOOP(idx, 2) for (auto & i : m_midiports[idx]) kCloseMidiPorts[idx](*this, i.b);


	////stop audio

	//REFLEX_ASSERT(!BitCheck(m_state, kStateStart));

	//CloseDriver();
}

Array < Pair <WString,bool> > WASAPI::GetMidiPorts(bool output) const
{
	return {};

	//Array < Pair <WString,bool> > rtn;

	//auto & ports = m_midiports[output];

	//rtn.Allocate(ports.GetSize());

	//for (auto & i : ports) rtn.Push({ i.a, True(i.b) });

	//return rtn;
}

void WASAPI::OnEnableMidiPort(bool output, UInt idx, bool enable)
{
	//auto & ports = m_midiports[output];

	//if (idx < ports.GetSize())
	//{
	//	Print(Common::log, "WASAPI::EnableMidiPort", output, ports[idx].a, enable);

	//	auto & port = ports[idx];

	//	kCloseMidiPorts[output](*this, port.b);

	//	if (enable) port.b = kOpenMidiPorts[output](*this, idx);
	//}
}

Array <WString> WASAPI::GetAvailableAudioDevices() const
{
	Array <WString> rtn;

	EnumerateDevices([&rtn](IMMDevice * device)
	{
		PROPVARIANT varName;

		Common::ComPtr <IPropertyStore> propertyStore;

		WASAPI_CHECK(device->OpenPropertyStore(STGM_READ, propertyStore.WriteAdr()), "OpenPropertyStore");

		PropVariantInit(&varName);

		WASAPI_CHECK(propertyStore->GetValue(PKEY_Device_FriendlyName, &varName), "GetValue");

		rtn.Push(varName.pwszVal);

		PropVariantClear(&varName);
	});
	
	return rtn;
}

bool WASAPI::OnSelectAudioDevice(const WString::View & value)
{
	try
	{
		EnumerateDevices([this, &value](IMMDevice * device)
		{
			if (GetDeviceName(device) == value)
			{
				m_audio_client.Clear();

				m_nchannel = 0;

				m_samplerate = 44100.0f;

				m_buffersize = 1024;

				m_current_device = value;

				m_device = device;

				throw(true);
			}
		});
	}
	catch (bool ok)
	{
		return ok;
	}

	return false;
}

void WASAPI::OnRequestBufferSize(UInt32 value)
{
	Print(Common::log, "WASAPI::SelectBufferSize ", value);

	m_buffersize = value;	//set this here temporarily, its used to request buffersize in OnResume and will also be set back to actual value in OnResume
}

void WASAPI::OnRequestSampleRate(Float32 value)
{
	//not implemented
	//m_samplerate = value;
}

Array <Float> WASAPI::GetAvailableSampleRates() const
{
	Array <Float> rtn;

	if (auto audio_client = m_audio_client)
	{
		for (auto & i : kStandardSampleRates)
		{
			WAVEFORMATEX waveFormat = { 0 };
			waveFormat.wFormatTag = WAVE_FORMAT_PCM;
			waveFormat.nChannels = 2; // Stereo
			waveFormat.nSamplesPerSec = Truncate(i);
			waveFormat.wBitsPerSample = 16; // 16-bit
			waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
			waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
			waveFormat.cbSize = 0;

			WAVEFORMATEX * out = 0;

			HRESULT hr = audio_client->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &waveFormat, &out);

			if (hr == S_OK)
			{
				rtn.Push(i);
			}
			else if (hr == S_FALSE)
			{
				out->nSamplesPerSec;
			}

			if (out) CoTaskMemFree(out);
		}
	}
	else
	{
		rtn = ToView(kStandardSampleRates);
	}

	return rtn;
}

Float32 WASAPI::GetCurrentSampleRate() const
{
	return m_samplerate;
}

UInt32 WASAPI::GetCurrentBufferSize() const
{
	return m_buffersize;
}

Array < Pair <WString, bool> > WASAPI::GetAudioChannels(bool output) const
{
	Array < Pair <WString, bool> > rtn;

	if (output)
	{
		rtn.Allocate(m_nchannel);

		REFLEX_LOOP(idx, m_nchannel)
		{
			rtn.Push({ Join(L"Channel ", ToWString(idx + 1)), true });
		}
	}

	return rtn;

	//Array < Pair <WString, bool> > rtn;

	//auto & channels = m_channels[output];

	//rtn.Allocate(channels.GetSize());

	//for (auto & i : channels) rtn.Push({ i.label, i.enabled });

	//return rtn;
}

void WASAPI::OnEnableAudioChannel(bool output, UInt32 idx, bool enable)
{
	//ProcessingLock lock(*this);

	//m_channels[output][idx].enabled = enable;
}

REFLEX_NOINLINE void WASAPI::OnError(const CString::View & error)
{
	Print(Common::log, "WASAPI Error", error);

	m_current_device = {};

	ClearBuffers();

	CloseDriver();

	//m_audioformat = { kAudioFormatUnknown, kAudioFormatUnknown };

	//st_processfn = kProcessFns[0][0];
}

void WASAPI::CloseDriver()
{
	//if (m_asio_current)
	//{
	//	if (BitCheck(m_state, kStateStart)) m_asio_current->stop();

	//	if (BitCheck(m_state, kStateBuffers)) m_asio_current->disposeBuffers();

	//	m_asio_current->Release();

	//	m_state = 0;

	//	m_asio_current = 0;
	//}

	//m_asio_buffers.Clear();
}

void WASAPI::OnPause()
{
	m_run.store(false);

	//m_thread->Wait();

	m_thread.Clear();

	if (m_event_handle) CloseHandle(m_event_handle);

	if (m_audio_client) m_audio_client->Stop();

	m_render_client.Clear();

	m_audio_client.Clear();

	m_event_handle = nullptr;
}

void WASAPI::OnResume()
{
	if (m_device)
	{
		try
		{
			WASAPI_CHECK(m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)m_audio_client.WriteAdr()), "Activate");


			WAVEFORMATEX * pwaveformat = nullptr;

			WASAPI_CHECK(m_audio_client->GetMixFormat(&pwaveformat), "GetMixFormat");

			WAVEFORMATEX waveformat = *pwaveformat;


			m_nchannel = waveformat.nChannels;

			m_samplerate = Float32(waveformat.nSamplesPerSec);

			REFERENCE_TIME bufferDuration = Truncate((Float64(m_buffersize) / Float64(m_samplerate)) * 10000000.0);

			WASAPI_CHECK(m_audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, bufferDuration, 0, pwaveformat, nullptr), "Inisalise");

			CoTaskMemFree(pwaveformat); // Free the mix format


			WASAPI_CHECK(m_audio_client->GetService(__uuidof(IAudioRenderClient), (void**)m_render_client.WriteAdr()), "GetService");

			WASAPI_CHECK(m_audio_client->GetBufferSize(&m_buffersize), "couldnt get buffersize");


			m_event_handle = CreateEvent(nullptr, FALSE, FALSE, nullptr);

			WASAPI_CHECK(m_audio_client->SetEventHandle(m_event_handle), "Failed to set event handle.");


			m_buffer.SetSize(m_buffersize * m_nchannel);

			m_buffer.Wipe();

			ClearBuffers();

			auto ptr = m_buffer.GetData();

			REFLEX_LOOP(idx, m_nchannel)
			{
				AddBuffer(true, ptr);

				ptr += m_buffersize;
			}

			PrepareProcessing(m_buffersize, m_samplerate);

			WASAPI_CHECK(m_audio_client->Start(), "Start");

			m_run.store(true);

			m_thread = Thread::Create([this, buffersize = m_buffersize, nchn = m_nchannel, buffers = ToView(GetBuffers(true))]()
			{
				BYTE* buffer = nullptr;
				UINT32 padding;

				LOOP:
				WaitForSingleObject(m_event_handle, INFINITE);

				if (m_run.load())
				{
					[[maybe_unused]] auto hr0 = m_audio_client->GetCurrentPadding(&padding);

					auto availableFrames = buffersize - padding;

					if (availableFrames > 0)
					{
						ProcessingScopeRt scope(this);

						if (!scope.IsLocked())
						{
							[[maybe_unused]] auto hr2 = m_render_client->GetBuffer(availableFrames, &buffer);

							ProcessRt(scope, availableFrames);

							if (nchn == 2)
							{
								auto pbuffer = Reinterpret<Pair<Float32>>(buffer);

								auto l = buffers[0];

								auto r = buffers[1];

								REFLEX_LOOP(idx, availableFrames)
								{
									*pbuffer++ = { *l++, *r++ };
								}
							}

							m_render_client->ReleaseBuffer(availableFrames, 0);
						}
					}

					goto LOOP;
				}
			
				if (m_audio_client) m_audio_client->Stop();
			});
		}
		catch (CString::View error)
		{
			OnError(error);
		}
	}
}

void WASAPI::EnumerateDevices(const Function <void(IMMDevice * ptr)> & callback) const
{
	try
	{
		Common::ComPtr <IMMDeviceEnumerator> enumerator;

		WASAPI_CHECK(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(enumerator.WriteAdr())), "CoCreateInstance");


		Common::ComPtr <IMMDeviceCollection> device_collection;

		WASAPI_CHECK(enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, device_collection.WriteAdr()), "EnumAudioEndpoints");


		UINT ndevice;

		WASAPI_CHECK(device_collection->GetCount(&ndevice), "GetCount");


		REFLEX_LOOP(idx, ndevice)
		{
			Common::ComPtr <IMMDevice> device;

			WASAPI_CHECK(device_collection->Item(idx, device.WriteAdr()), "Item");

			device->AddRef();

			callback(device);
		}
	}
	catch (CString::View e)
	{
		RemoveConst(this)->OnError(e);
	}
}

WString WASAPI::GetDeviceName(IMMDevice * device) const
{
	WString rtn;

	try
	{
		PROPVARIANT varName;

		Common::ComPtr <IPropertyStore> propertyStore;

		WASAPI_CHECK(device->OpenPropertyStore(STGM_READ, propertyStore.WriteAdr()), "OpenPropertyStore");

		PropVariantInit(&varName);

		WASAPI_CHECK(propertyStore->GetValue(PKEY_Device_FriendlyName, &varName), "GetValue");

		rtn = varName.pwszVal;

		PropVariantClear(&varName);
	}
	catch (CString::View error)
	{
		RemoveConst(this)->OnError(error);
	}

	return rtn;
}

//template <bool OUTPUT> HANDLE WASAPI::OpenMidiPort(WASAPI & self, UInt idx)
//{
//	HANDLE handle = 0;
//
//	if constexpr (OUTPUT)
//	{
//		HMIDIOUT midiport = 0;
//
//		midiOutOpen(&midiport, idx, 0, 0, CALLBACK_NULL);
//
//		handle = midiport;
//	}
//	else
//	{
//		HMIDIIN midiport = 0;
//
//		if (midiInOpen(&midiport, idx, DWORD_PTR(&WASAPI::MidiInProc), idx, CALLBACK_FUNCTION) == MMSYSERR_NOERROR)
//		{
//			midiInStart(midiport);
//		}
//
//		handle = midiport;
//	}
//
//	auto & active = self.m_active_midiports[OUTPUT];
//
//	if (!SearchValue(active, handle)) active.Push(handle);
//
//	return handle;
//}

template <bool OUTPUT> void WASAPI::CloseMidiPort(WASAPI & self, HANDLE & handle)
{
	//auto midiport = HMIDIIN(handle);

	//RemoveValue(self.m_active_midiports[OUTPUT], handle);

	//if constexpr (OUTPUT)
	//{
	//	midiOutClose(reinterpret_cast<HMIDIOUT&>(midiport));
	//}
	//else
	//{
	//	midiInReset(midiport);

	//	midiInClose(midiport);
	//}

	//handle = 0;
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::Common::DesktopAudioAppBase> Reflex::System::Win::CreateASIO()
{
	return REFLEX_CREATE(WASAPI);
}

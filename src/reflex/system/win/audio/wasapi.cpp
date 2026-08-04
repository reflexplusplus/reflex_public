#include "../[require].h"

#include "../library.h"
#include "../ms_com.h"
#include "../../common/instance/audioapp.h"
#include "../../common/utf8.h"

#include <audioclient.h>
#include <mmdeviceapi.h>
#include <avrt.h>
#include <Functiondiscoverykeys_devpkey.h>
#pragma comment(lib, "avrt.lib")




REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

struct WasapiDevice : public AudioDevice
{
	template <class TYPE> using ComPtr = COM::Reference <TYPE>;


	static void EnumerateEndpoints(EDataFlow flow, Type baseFlags, Array <AudioDevice::Desc> & out);

	
	WasapiDevice(Type type, const ArrayView <UInt8> & id)
		: m_type(type)
		, m_id(id)
		, m_num_channel(0)
		, m_event(CreateEvent(nullptr, FALSE, FALSE, nullptr))
	{
		REFLEX_ASSERT_MAINTHREAD("System::Win::WasapiDevice::WasapiDevice");	//otherwise we need CoInit / Deinit per thread

		ComPtr <IMMDeviceEnumerator> enumerator;

		auto hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)enumerator.WriteAdr());

		if (SUCCEEDED(hr))
		{
			auto id = System::Common::DecodeUTF8(m_id);

			enumerator->GetDevice(id.GetData(), m_device.WriteAdr());
		}
	}

	~WasapiDevice()
	{
		Stop();
		
		if (m_event) CloseHandle(m_event);

		m_capture_or_render.Clear();

		m_client.Clear();
		m_device.Clear();
	}

	virtual void OnAllocateBuffers(UInt num_channel, UInt maxbuffersize) = 0;

	virtual void OnRunRt(bool exclusive) = 0;

	bool Init(Object & client, const Config & config) override
	{
		Stop();

		m_client.Clear();

		try
		{
			m_owner = client;

			if (!m_event) throw(false);

			if (!m_device) throw(false);

			UInt16 num_channel[2] = { config.num_input, config.num_output };

			if (num_channel[m_type == kTypeInput]) throw(false);

			MS_TRY(m_device->Activate, (__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)m_client.WriteAdr()));

			WAVEFORMATEX * mix_fmt = nullptr;

			MS_TRY(m_client->GetMixFormat, (&mix_fmt));

			Array <UInt8> buffer(sizeof(WAVEFORMATEX) + mix_fmt->cbSize);

			MemCopy(mix_fmt, buffer.GetData(), buffer.GetSize());

			CoTaskMemFree(mix_fmt);

			auto & fmt = *Reinterpret<WAVEFORMATEX>(buffer.GetData());

			WAVEFORMATEX original = fmt;

			if (fmt.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
			{
				auto ext = Reinterpret<WAVEFORMATEXTENSIBLE>(&fmt);

				if (ext->SubFormat != KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) throw(false);
			}
			else
			{
				if (fmt.wFormatTag != WAVE_FORMAT_IEEE_FLOAT) throw(false);
			}

			fmt.nChannels = num_channel[m_type];
			fmt.wBitsPerSample = 32;
			fmt.nBlockAlign = (fmt.wBitsPerSample / 8) * fmt.nChannels;
			fmt.nSamplesPerSec = Truncate(config.sample_rate);
			fmt.nAvgBytesPerSec = fmt.nBlockAlign * fmt.nSamplesPerSec;

			m_num_channel = fmt.nChannels;

			m_samplerate = Float32(fmt.nSamplesPerSec);

			
			UInt64 buffer_duration = 0;

			if (config.exclusive)
			{
				m_client_callback = reinterpret_cast<decltype(m_client_callback)>(config.interleaved_16bit);

				WAVEFORMATEX desired = {};
				desired.wFormatTag = WAVE_FORMAT_PCM;
				desired.nChannels = fmt.nChannels;        // keep same channel count
				desired.nSamplesPerSec = fmt.nSamplesPerSec;   // keep same sample rate
				desired.wBitsPerSample = 16;                   // safe bet
				desired.nBlockAlign = (desired.wBitsPerSample / 8) * desired.nChannels;
				desired.nAvgBytesPerSec = desired.nBlockAlign * desired.nSamplesPerSec;

				WAVEFORMATEX * closest = nullptr;

				MS_TRY(m_client->IsFormatSupported, (AUDCLNT_SHAREMODE_EXCLUSIVE, &desired, &closest));

				if (closest)
				{
					fmt = *closest;

					CoTaskMemFree(closest);
				}
				else
				{
					fmt = desired;
				}

				buffer_duration = (REFERENCE_TIME)((10'000'000.0 * config.buffer_size) / fmt.nSamplesPerSec);
			}
			else
			{
				m_client_callback = reinterpret_cast<decltype(m_client_callback)>(config.interleaved_32bit);
			}

			if (!m_client_callback) throw(false);

			MS_TRY(m_client->Initialize, (config.exclusive ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, buffer_duration, buffer_duration, &fmt, nullptr));

			MS_TRY(m_client->SetEventHandle, (m_event));
			
			UINT32 buffersize;
		
			MS_TRY(m_client->GetBufferSize, (&buffersize));

			m_maxbuffersize = buffersize;

			OnAllocateBuffers(m_num_channel, m_maxbuffersize);

			m_exclusive = config.exclusive;

			MS_TRY(m_client->GetService, (kGUIDS[m_type], (void**)m_capture_or_render.WriteAdr()));

			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	Array <Float> GetAvailableSampleRates() const override
	{
		Array <Float> rtn;

		try
		{
			if (!m_device) throw(false);

			ComPtr <IAudioClient> client;
			
			WAVEFORMATEX * mix_fmt = nullptr;

			MS_TRY(RemoveConst(*m_device).Activate, (__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)client.WriteAdr()));

			MS_TRY(client->GetMixFormat, (&mix_fmt));

			REFLEX_FOREACH(rate, Common::AudioAppBase::kStandardSampleRates)
			{
				WAVEFORMATEX fmt = *mix_fmt;
				fmt.nSamplesPerSec = Truncate(rate + 0.25f);
				fmt.nAvgBytesPerSec = fmt.nBlockAlign * fmt.nSamplesPerSec;

				WAVEFORMATEX * closest = nullptr;
				HRESULT hr = client->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &fmt, &closest);

				if (hr == S_OK) rtn.Push(rate); // or out.push_back(rate);

				if (closest) CoTaskMemFree(closest);
			}

			CoTaskMemFree(mix_fmt);
		}
		catch (...)
		{
		}

		return rtn;
	}

	Array <UInt32> GetAvailableBufferSizes() const override
	{
		return { 512, 1024, 2048 };
	}

	ArrayView <UInt8> GetID() const override { return m_id; }

	Float32 GetSampleRate() const override { return m_samplerate; }
	
	UInt32 GetBufferSize() const override { return m_maxbuffersize; }

	bool Start() override
	{
		if (SetFiltered(m_running, true) && m_client && m_capture_or_render)
		{
			if (FAILED(m_client->Start()))
			{
				goto Fail;
			}

			m_thread = CreateThread(nullptr, 0, &ThreadProc, this, 0, nullptr);

			if (!m_thread)
			{
				m_client->Stop();

				goto Fail;
			}

			return true;
		}

		REFLEX_MARKER(Fail);

		m_running = false;

		return false;
	}

	void Stop() override
	{
		if (SetFiltered(m_running, false))
		{
			if (m_event) SetEvent(m_event);

			if (auto thread = Poll(m_thread))
			{
				WaitForSingleObject(thread, INFINITE);
			
				CloseHandle(thread);
			}

			if (m_client) m_client->Stop();
		}
	}

	static DWORD WINAPI ThreadProc(LPVOID param)
	{
		CoInitializeEx(nullptr, COINIT_MULTITHREADED);

		auto self = static_cast<WasapiDevice*>(param);

		self->OnRunRt(self->m_exclusive);

		CoUninitialize();

		return 0;
	}


	const Type m_type;

	const Array <UInt8> m_id;

	HANDLE m_event;


	TRef <Object> m_owner;

	FunctionPointer <void(Object & owner, UInt samples, const void * inputs, void * outputs)> m_client_callback = nullptr;

	ComPtr <IUnknown> m_capture_or_render;
	ComPtr <IMMDevice> m_device;
	ComPtr <IAudioClient> m_client;

	UInt m_num_channel;
	Float32 m_samplerate = 1.0f;
	UInt32 m_maxbuffersize = 0;
	bool m_exclusive = false;

	bool m_running = false;

	HANDLE m_thread = nullptr;


	static inline const GUID kGUIDS[2] = { __uuidof(IAudioCaptureClient), __uuidof(IAudioRenderClient) };
};

struct WasapiOutputDevice : public WasapiDevice
{
	WasapiOutputDevice(const Array <UInt8> & id)
		: WasapiDevice(kTypeOutput, id)
	{
	}

	void OnAllocateBuffers(UInt num_channel, UInt maxbuffersize) override
	{
	}

	void OnRunRt(bool exclusive) override
	{
		auto irender = Cast<IAudioRenderClient>(Cast<IUnknown>(m_capture_or_render.Get()));

		UINT32 padding = 0;
		BYTE * data = nullptr;

		if (exclusive)
		{
			while (m_running)
			{
				DWORD wait = WaitForSingleObject(m_event, 2000);

				if (wait != WAIT_OBJECT_0) continue;

				if (FAILED(irender->GetBuffer(m_maxbuffersize, &data))) continue;

				m_client_callback(m_owner, m_maxbuffersize, nullptr, data);

				irender->ReleaseBuffer(m_maxbuffersize, 0);
			}
		}
		else
		{
			while (m_running)
			{
				DWORD wait = WaitForSingleObject(m_event, 2000);

				if (wait != WAIT_OBJECT_0) continue;

				if (FAILED(m_client->GetCurrentPadding(&padding))) continue;

				if (auto frames = m_maxbuffersize - padding)
				{
					REFLEX_ASSERT(frames <= m_maxbuffersize);

					if (FAILED(irender->GetBuffer(frames, &data))) continue;

					m_client_callback(m_owner, frames, nullptr, data);

					irender->ReleaseBuffer(frames, 0);
				}
			}
		}
	}
};

struct WasapiInputDevice : public WasapiDevice
{
	WasapiInputDevice(const Array <UInt8> & id)
		: WasapiDevice(kTypeInput, id)
	{
	}

	void OnAllocateBuffers(UInt num_channel, UInt maxbuffersize) override
	{
		m_silence.SetSize(num_channel * maxbuffersize);

		m_silence.Wipe();

		//m_inputs.SetSize(num_channel);

		//auto buffer = m_uninterleaved.GetData();

		//REFLEX_LOOP_PTR(m_inputs.GetData(), ptr, num_channel)
		//{
		//	*ptr = buffer;

		//	buffer += maxbuffersize;
		//}
	}

	void OnRunRt(bool exclusive)
	{
		auto icapture = Cast<IAudioCaptureClient>(Cast<IUnknown>(m_capture_or_render.Get()));

		while (m_running)
		{
			DWORD wait = WaitForSingleObject(m_event, 2000);

			if (wait != WAIT_OBJECT_0) continue;

			UINT32 packet_frames = 0;
			BYTE * data = nullptr;
			UINT32 frames = 0;
			DWORD flags = 0;

			HRESULT hr = icapture->GetNextPacketSize(&packet_frames);
			if (FAILED(hr)) continue;

			while (packet_frames != 0)
			{
				hr = icapture->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
				
				if (FAILED(hr)) break;

				REFLEX_ASSERT(frames <= m_maxbuffersize);

				if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
				{
					reinterpret_cast<Float*&>(data) = m_silence.GetData();
				}

				m_client_callback(m_owner, frames, data, nullptr);

				icapture->ReleaseBuffer(frames);

				hr = icapture->GetNextPacketSize(&packet_frames);
				if (FAILED(hr)) break;
			}
		}
	}


	Array <Float> m_silence;

	//Array <const Float*> m_inputs;
};

void WasapiDevice::EnumerateEndpoints(EDataFlow flow, Type baseFlags, Array <AudioDevice::Desc> & out)
{
	REFLEX_ASSERT_MAINTHREAD("System::Win::WasapiDevice::EnumerateEndpoints");	//otherwise we need CoInit / Deinit per thread

	constexpr auto GetDeviceId = [](IMMDevice * device)
	{
		Array <UInt8> rtn;

		LPWSTR id_w = nullptr;
		
		if (SUCCEEDED(device->GetId(&id_w)))
		{
			Data::EncodeUTF8(rtn, id_w);

			CoTaskMemFree(id_w);
		}

		return rtn;
	};

	ComPtr <IMMDeviceEnumerator> enumerator;
	ComPtr <IMMDeviceCollection> collection;
	UINT count = 0;

	try
	{
		MS_TRY(CoCreateInstance, (__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)enumerator.WriteAdr()));

		MS_TRY(enumerator->EnumAudioEndpoints, (flow, DEVICE_STATE_ACTIVE, collection.WriteAdr()));

		MS_TRY(collection->GetCount, (&count));
	}
	catch (...)
	{
		return;
	}

	ComPtr <IMMDevice> defaultDevice;

	Array <UInt8> default_id;

	if (SUCCEEDED(enumerator->GetDefaultAudioEndpoint(flow, eConsole, defaultDevice.WriteAdr())))
	{
		default_id = GetDeviceId(defaultDevice);
	}

	for (UINT i = 0; i < count; ++i)
	{
		ComPtr <IMMDevice> device;
		ComPtr <IPropertyStore> props;
		PROPVARIANT varName;
		WString device_id, name;

		Desc desc;
		desc.type = baseFlags;

		if (FAILED(collection->Item(i, device.WriteAdr()))) continue;

		desc.id = GetDeviceId(device);

		if (FAILED(device->OpenPropertyStore(STGM_READ, props.WriteAdr()))) continue;

		PropVariantInit(&varName);

		if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &varName)))
		{
			if (varName.vt == VT_LPWSTR && varName.pwszVal) desc.name = varName.pwszVal;
		}

		PropVariantClear(&varName);

		//if (flow == eRender)
		//{
		//	desc.max_channels[0] = 0;
		//	desc.max_channels[1] = 2;
		//}
		//else
		//{
		//	desc.max_channels[0] = 1;
		//	desc.max_channels[1] = 0;
		//}

		desc.is_default = (desc.id == default_id);

		out.Push(desc);
	}
}

REFLEX_END_INTERNAL

Reflex::Array <Reflex::System::AudioDevice::Desc > Reflex::System::AudioDevice::GetAvailableDevices()
{
	Array <AudioDevice::Desc> out;

	Win::WasapiDevice::EnumerateEndpoints(eRender, kTypeOutput, out);

	Win::WasapiDevice::EnumerateEndpoints(eCapture, kTypeInput, out);

	return out;
}

Reflex::TRef <Reflex::System::AudioDevice> Reflex::System::AudioDevice::Create(Type type, const Array <UInt8> & id)
{
	switch (type)
	{
	case kTypeOutput:
		return REFLEX_CREATE(Win::WasapiOutputDevice, id);

	case kTypeInput:
		return REFLEX_CREATE(Win::WasapiInputDevice, id);
	}

	return kNoValue;	//TODO NULL 
}

#include "coreaudio.h"
#include "globals.h"
#include "../common/instance/eventqueue.h"




//
//core audio

#define IS_OK(rtn) (rtn == noErr)
#define IS_ERROR(rtn) (rtn != noErr)
#define LOG_ERROR(a, ...) if (IS_ERROR(a(__VA_ARGS__))) { DEV_LOG("CoreAudio error:", REFLEX_STRINGIFY(a)); }

REFLEX_BEGIN_INTERNAL(Reflex::System::OSX)

class CoreAudio : public Common::DesktopAudioAppBase
{
public:

	//info

	struct AudioDeviceInfo
	{
		AudioDeviceID deviceid;

		WString name;

		bool default_output = false;
	};

	static void EnumerateAudioDevices(void * _Nullable client, FunctionPointer<bool(void*,const AudioDeviceInfo &)> _Nonnull callback);


	//lifetime

	CoreAudio();



private:

	struct MidiPort;

	struct Channel;


	virtual void OnPause() override;

	virtual void OnResume() override;


	virtual Array < Pair<WString,bool> > GetMidiPorts(bool output) const override;

	virtual void OnEnableMidiPort(bool output, UInt idx, bool enable) override;


	virtual Array <WString> GetAvailableAudioDevices() const override;

	virtual bool OnSelectAudioDevice(const WString::View & value) override;

	virtual WString GetCurrentAudioDevice() const override;

	virtual UInt32 GetCurrentBufferSize() const override { return m_buffersize; }

	virtual Float32 GetCurrentSampleRate() const override { return m_samplerate; }


	virtual void OnRequestBufferSize(UInt32 value) override;

	virtual void OnRequestSampleRate(Float32 value) override;


	virtual Array <UInt32> GetAvailableBufferSizes() const override;

	virtual Array <Float32> GetAvailableSampleRates() const override;


	virtual Array < Pair <WString,bool> > GetAudioChannels(bool output) const override;

	virtual void OnEnableAudioChannel(bool output, UInt32 idx, bool enable) override;


	static void OnClock(CoreAudio & self);

	static void GetMidiPortName(const MIDIEndpointRef & midiendpointref, UInt32 idx, WString & string);

	template <bool OUTPUT> static void CloseMidiPort(CoreAudio & self, MidiPort & port);

	template <bool OUTPUT> static void OpenMidiPort(CoreAudio & self, MidiPort & port);


	void UpdateBuffers();

	UInt32 GetDeviceBufferSize();
	
	Float32 GetDeviceSampleRate();
	

	static void ProcessMidiInCallback(const MIDIPacketList *pktlist, void *readProcRefCon, void *srcConnRefCon);


	static OSStatus ProcessAudioCallbackRt(AudioDeviceID device, const AudioTimeStamp* now, const AudioBufferList * input, const AudioTimeStamp * intime, AudioBufferList * output, const AudioTimeStamp * outtime, void * client);

	void ProcessRt(const AudioBufferList * input, AudioBufferList * output);


	static void ProcessNullInputRt(const AudioBuffer & audiobuffer, Channel * channel, UInt32 buffersize) {}

	static void ProcessMonoInputRt(const AudioBuffer & audiobuffer, Channel * channel, UInt32 buffersize);

	static void ProcessStereoInputRt(const AudioBuffer & audiobuffer, Channel * channel, UInt32 buffersize);

	static void ProcessInputsRt(const AudioBuffer & audiobuffer, Channel * channel, UInt32 buffersize);


	static void ProcessNullOutputRt(const Channel * channels, AudioBuffer & audiobuffer, UInt32 buffersize) {}

	static void ProcessMonoOutputRt(const Channel * channels, AudioBuffer & audiobuffer, UInt32 buffersize);

	static void ProcessStereoOutputRt(const Channel * channels, AudioBuffer & audiobuffer, UInt32 buffersize);

	template <UInt N> static void ProcessOutputsRt(const Channel * channels, AudioBuffer & audiobuffer, UInt32 buffersize);


	static OSStatus AudioDeviceGetPropertyInfo(AudioDeviceID  inDevice, UInt32 inChannel, Boolean isInput, AudioDevicePropertyID inPropertyID, UInt32* __nullable outSize, Boolean* __nullable outWritable);

	static OSStatus AudioDeviceGetProperty(AudioDeviceID device, UInt32 chn, Boolean is_input, AudioDevicePropertyID property_id, UInt32 * io_size, void * out_data);

	static OSStatus AudioDeviceSetProperty(AudioDeviceID device, int unused, UInt32 chn, Boolean is_input, AudioDevicePropertyID property_id, UInt32 data_size, const void * data);


	//midi

	struct MidiFns
	{
		decltype (&CloseMidiPort<false>) ClosePort;
		decltype (&OpenMidiPort<false>) OpenPort;
		//decltype (&MIDIGetNumberOfSources) GetCount;
		decltype (&MIDIGetSource) GetRef;
	};

	struct MidiPort
	{
		UInt16 idx;
		MIDIEndpointRef deviceid;
		MIDIClientRef clientref;
		MIDIPortRef portref;
	};

	typedef Array <Pair<WString,MidiPort>> MidiPortList;


	Common::Signal & changedevices_signal;


	MidiFns m_midifns[2];

	MidiPortList m_midiports[2];

	Reference <Object> m_onclock;

	UInt m_clockindex;
	
	bool m_started;



	//audio

	Array <AudioDeviceInfo> m_audiodevices;

	AudioDeviceInfo m_currentdevice;


	struct Channel
	{
		AudioDeviceID deviceid;
		WString name;
		bool enabled;
		Float32 * buffer;
	};

	Array <Channel> m_channels[2];


	UInt m_buffersize;

	Float32 m_samplerate;


	Array <Float32> m_silence_buffer;

	Array < Array <Float32> > m_buffers[2];


	AudioDeviceIOProcID m_audio_procid;


	static CoreAudio * st_self;

	static const Float64 kSampleRates[5];

	static const UInt32 kMidiTypeToSize[16];

	static const FunctionPointer <void(const AudioBuffer&,Channel*,UInt)> kProcessInputFns[4];

	static const FunctionPointer <void(const Channel*,AudioBuffer&,UInt)> kProcessOutputFns[18];

};

CoreAudio::CoreAudio()
	: changedevices_signal(globals->m_signals[kNotificationChangeDevices])
	, m_clockindex(-1)
	, m_started(false)
	, m_buffersize(kDefaultBufferSize)
	, m_samplerate(kDefaultSampleRate)
{
	st_self = this;

	auto & midiinfns = m_midifns[0];

	midiinfns.GetRef = &MIDIGetSource;
	midiinfns.ClosePort = &CloseMidiPort<false>;
	midiinfns.OpenPort = &OpenMidiPort<false>;

	auto & midioutfns = m_midifns[1];

	midioutfns.GetRef = &MIDIGetDestination;
	midioutfns.ClosePort = &CloseMidiPort<true>;
	midioutfns.OpenPort = &OpenMidiPort<true>;


	m_currentdevice.deviceid = 0;

	m_audiodevices.Allocate(8);

	EnumerateAudioDevices(this, [](void * client, const AudioDeviceInfo & info)
	{
		Cast<CoreAudio>(client)->m_audiodevices.Push(info);

		return true;
	});


	m_onclock = CreateListener(kNotificationClock, this, reinterpret_cast<FunctionPointer<void(void*)>>(&CoreAudio::OnClock));
}

Array < Pair <WString,bool> > CoreAudio::GetMidiPorts(bool output) const
{
	Array < Pair <WString,bool> > rtn;

	rtn.Allocate(m_midiports[output].GetSize());

	for (auto & i : m_midiports[output]) rtn.Push<kAllocateNone>({i.a,True(i.b.portref)});

	return rtn;
}

template <bool OUTPUT> void CoreAudio::OpenMidiPort(CoreAudio & self, MidiPort & port)
{
	MIDIClientCreate(OUTPUT ? CFSTR("MIDI_OUT_CLIENT") : CFSTR("MIDI_IN_CLIENT"), NULL, NULL, &port.clientref);

	if (OUTPUT)
	{
		MIDIOutputPortCreate(port.clientref, CFSTR("MIDI_OUT"), &port.portref);
	}
	else
	{
		MIDIInputPortCreate(port.clientref, CFSTR("MIDI_IN"), ProcessMidiInCallback, &port, &port.portref);

		MIDIPortConnectSource(port.portref, port.deviceid, 0);
	}
}

template <bool OUTPUT> void CoreAudio::CloseMidiPort(CoreAudio & self, MidiPort & port)
{
	if (!OUTPUT) MIDIPortDisconnectSource(port.portref, port.deviceid);

	MIDIPortDispose(port.portref);

	MIDIClientDispose(port.clientref);

	port.portref = 0;

	port.clientref = 0;
}

void CoreAudio::OnEnableMidiPort(bool output, UInt idx, bool enable)
{
	auto & ports = m_midiports[output];

	if (idx < ports.GetSize())
	{
		auto & midifns = m_midifns[output];

		auto & port = ports[idx].b;

		if (port.clientref)
		{
			midifns.ClosePort(*this, port);
		}

		if (enable)
		{
			midifns.OpenPort(*this, port);
		}
	}
}

Array <WString> CoreAudio::GetAvailableAudioDevices() const
{
	Array <WString> rtn;

	for (auto & i : m_audiodevices) rtn.Push(i.name);

	return rtn;
}

bool CoreAudio::OnSelectAudioDevice(const WString::View & name)
{
	for (auto & device : m_audiodevices)
	{
		if (device.name == name)
		{
			m_currentdevice = device;

			::UInt32 size = 0;

			UInt32 channelcount[2] = {0, 0};

			REFLEX_LOOP(output, 2)
			{
				OSStatus error;

				size = 0;

				bool input = Not(output);

				AudioDeviceGetPropertyInfo(device.deviceid, 0, input, kAudioDevicePropertyStreamConfiguration, &size, NULL);

				Array <UInt8> buffer(size);

				AudioBufferList & audiobufferlist = *reinterpret_cast<AudioBufferList*>(buffer.GetData());

				error = AudioDeviceGetProperty(device.deviceid, 0, input, kAudioDevicePropertyStreamConfiguration, &size, &audiobufferlist);

				auto & channels = m_channels[output];

				channels.Clear();

				REFLEX_LOOP(buffer, audiobufferlist.mNumberBuffers)
				{
					auto & audiobuffer = audiobufferlist.mBuffers[buffer];

					UInt32 nchannel = audiobuffer.mNumberChannels;

					REFLEX_LOOP(chn, nchannel)
					{
						auto & channel = channels.Push();

						UInt32 channelidx = channelcount[input]++;

						channel.deviceid = device.deviceid;

						channel.name = Join(device.name, L' ', Reflex::ToWString(channelidx + 1));

						channel.enabled = false;
					}
				}
			}

			m_samplerate = GetDeviceSampleRate();
			
			m_buffersize = GetDeviceBufferSize();

			return true;
		}
	}

	return false;
}

void CoreAudio::OnRequestBufferSize(UInt32 value)
{
	if (IS_ERROR(AudioDeviceSetProperty(m_currentdevice.deviceid, 0, 0, false, kAudioDevicePropertyBufferFrameSize, sizeof(UInt32), &value)))
	{
		value = GetDeviceBufferSize();
	}

	m_buffersize = value;
}

void CoreAudio::OnRequestSampleRate(Float32 value)
{
	Float64 samplerate = value;

	if (IS_ERROR(AudioDeviceSetProperty(m_currentdevice.deviceid, 0, 0, false, kAudioDevicePropertyNominalSampleRate, sizeof(Float64), &samplerate)))
	{
		samplerate = GetDeviceSampleRate();
	}

	m_samplerate = Float32(samplerate);
}

WString CoreAudio::GetCurrentAudioDevice() const
{
	return m_currentdevice.name;
}

Array <UInt32> CoreAudio::GetAvailableBufferSizes() const
{
	Array <UInt32> rtn;

	if (auto deviceid = m_currentdevice.deviceid)
	{
		AudioValueRange buffersizes;
		
		::UInt32 size = sizeof(AudioValueRange);
		
		AudioDeviceGetProperty(deviceid, 0, false, kAudioDevicePropertyBufferFrameSizeRange, &size, &buffersizes);
		
		UInt32 buffersize = Max(RoundUpPow2(ToInt32(buffersizes.mMinimum)), 64);
		
		UInt32 max = Min(ToInt32(buffersizes.mMaximum), 2048);
		
		while (buffersize <= max)
		{
			rtn.Push(buffersize);
			
			buffersize *= 2;
		}
		
		if (rtn.Empty()) rtn.Push(kDefaultBufferSize);
	}
	
	return rtn;
}

Array <Float32> CoreAudio::GetAvailableSampleRates() const
{
	Array <Float32> rtn;
	
	if (auto deviceid = m_currentdevice.deviceid)
	{
		::UInt32 size = 0;

		AudioDeviceGetPropertyInfo(deviceid, 0, false, kAudioDevicePropertyAvailableNominalSampleRates, &size, NULL);
		
		Array <AudioValueRange> samplerates(size / sizeof(AudioValueRange));
		
		AudioDeviceGetProperty(deviceid, 0, false, kAudioDevicePropertyAvailableNominalSampleRates, &size, samplerates.GetData());
				
		for (auto & i : samplerates)
		{
			Float64 sr = i.mMinimum;
			
			if (Search(ToView(kSampleRates), sr)) rtn.Push(sr);
		}
		
		if (samplerates.Empty()) rtn.Push(kDefaultSampleRate);
	}
	
	return rtn;
}

Array < Pair<WString,bool> > CoreAudio::GetAudioChannels(bool output) const
{
	Array < Pair<WString,bool> > rtn;

	auto & channels = m_channels[output];

	rtn.Allocate(channels.GetSize());

	for (auto & i : channels) rtn.Push<kAllocateNone>({ i.name, i.enabled });

	return rtn;
}

void CoreAudio::OnEnableAudioChannel(bool output, UInt32 idx, bool enable)
{
	m_channels[output][idx].enabled = enable;
}

void CoreAudio::OnPause()
{
	while (REFLEX_ATOMIC_READ(m_isprocessing_rt))	//dont know why this is needed, but users card freezes otherwise
	{
		Common::output.Log("CoreAudio waiting to stop (1)...");

		SuspendThread(20);
	}
	
	LOG_ERROR(AudioDeviceStop, m_currentdevice.deviceid, m_audio_procid);
		
	
	
	//AudioDeviceStop is non blocking

	AtomicUInt32 max_tries = 16;
	
	::UInt32 size = sizeof(UInt32);
	
	UInt32 running = 0;
	
	AudioObjectPropertyAddress pa = {kAudioDevicePropertyDeviceIsRunning, kAudioObjectPropertyScopeWildcard, kAudioObjectPropertyElementMaster};
	
	AudioObjectGetPropertyData(m_currentdevice.deviceid, &pa, 0, 0, &size, &running);
	
	while (running && max_tries--)
	{
		Common::output.Log("CoreAudio waiting to stop (2)...");

		SuspendThread(20);
		
		AudioObjectGetPropertyData(m_currentdevice.deviceid, &pa, 0, 0, &size, &running);
	}
	
	if (running) Common::output.Warn("CoreAudio driver reported still running");

	
	
	//TODO try moving here
	
	LOG_ERROR(AudioDeviceDestroyIOProcID, m_currentdevice.deviceid, m_audio_procid);
}

void CoreAudio::OnResume()
{
	if (SetFiltered(m_started, true))
	{
		//HACK FOR FIRST TIME

		OnClock(*this);
	}
	
	UpdateBuffers();

	PrepareProcessing(m_buffersize, m_samplerate);

	m_audio_procid = 0;

	if (IS_OK(AudioDeviceCreateIOProcID(m_currentdevice.deviceid, &CoreAudio::ProcessAudioCallbackRt, this, &m_audio_procid)))
	{
		LOG_ERROR(AudioDeviceStart, m_currentdevice.deviceid, m_audio_procid);

		::UInt32 size = sizeof(UInt32);

		UInt32 running = 0;

		AudioObjectPropertyAddress pa = {kAudioDevicePropertyDeviceIsRunning, kAudioObjectPropertyScopeWildcard, kAudioObjectPropertyElementMaster};

		AudioObjectGetPropertyData(m_currentdevice.deviceid, &pa, 0, 0, &size, &running);

		while (!running)
		{
			Common::output.Log("CoreAudio waiting to start...");

			SuspendThread(20);

			AudioObjectGetPropertyData(m_currentdevice.deviceid, &pa, 0, 0, &size, &running);
		}
	}
	else
	{
		Common::output.Error("CoreAudio AudioDeviceCreateIOProcID failed");
	}
}

void CoreAudio::OnClock(CoreAudio & self)
{
	self.m_clockindex = (self.m_clockindex + 1) & 31;

	if (!self.m_clockindex)
	{
		bool changed = false;

		UInt nports[] = { UInt(MIDIGetNumberOfSources()), UInt(MIDIGetNumberOfDestinations()) };

		REFLEX_LOOP(output, 2) changed = Or(changed, self.m_midiports[output].GetSize() != nports[output]);

		if (changed)
		{
			Lock lock(self);

			Array <WString> open;

			REFLEX_LOOP(output, 2)
			{
				auto & midifns = self.m_midifns[output];

				auto & ports = self.m_midiports[output];

				open.Clear();

				for (auto & i : ports)
				{
					if (i.b.clientref)
					{
						open.Push(i.a);

						midifns.ClosePort(self, i.b);
					}
				}

				UInt nport = nports[output];

				ports.SetSize(nport);//important as dealing with ptr in OpenPort

				REFLEX_LOOP(idx, nport)
				{
					MIDIEndpointRef midiref = midifns.GetRef(idx);

					auto & pair = ports[idx];

					GetMidiPortName(midiref, idx, pair.a);

					pair.b.idx = idx;

					pair.b.deviceid = midiref;

					pair.b.clientref = 0;

					pair.b.portref = 0;

					if (Search(open, pair.a)) midifns.OpenPort(self, pair.b);
				}
			}

			self.changedevices_signal.Notify();
		}
	}
}

void CoreAudio::UpdateBuffers()
{
	ClearBuffers();

	m_silence_buffer.SetSize(m_buffersize);

	m_silence_buffer.Wipe();

	REFLEX_LOOP(output, 2)
	{
		auto & channels = m_channels[output];

		auto & buffers = m_buffers[output];

		UInt32 nchannel = channels.GetSize();

		buffers.SetSize(nchannel);

		REFLEX_LOOP(idx, nchannel)
		{
			auto & channel = channels[idx];

			auto & buffer = buffers[idx];

			if (channel.enabled)
			{
				buffer.SetSize(m_buffersize);

				buffer.Wipe();

				channel.buffer = buffer.GetData();

				AddBuffer(output, channel.buffer);
			}
			else
			{
				buffer.Clear();

				channel.buffer = m_silence_buffer.GetData();
			}
		}
	}
}

UInt32 CoreAudio::GetDeviceBufferSize()
{
	::UInt32 size = sizeof(UInt32);

	UInt32 value = 512;
	
	AudioDeviceGetProperty(m_currentdevice.deviceid, 0, false, kAudioDevicePropertyBufferFrameSize, &size, &value);

	return value ? value : 512;
}

Float32 CoreAudio::GetDeviceSampleRate()
{
	::UInt32 size = sizeof(Float64);

	Float64 samplerate = kDefaultSampleRate;
	
	AudioDeviceGetProperty(m_currentdevice.deviceid, 0, false, kAudioDevicePropertyNominalSampleRate, &size, &samplerate);
	
	return samplerate ? Float32(samplerate) : kDefaultSampleRate;
}

void CoreAudio::ProcessMidiInCallback(const MIDIPacketList * packetlist, void * client, void * source)
{
	constexpr auto QueueShortMessage = [](CoreAudio & self, MidiPort & port, const UInt8 * src, UInt size)
	{
		UInt32 midimsg = 0;

		MemCopy(src, &midimsg, size);

		self.QueueEvent(Event::kTypeMIDI, 0, port.idx, midimsg);
	};

	auto & self = *st_self;

	auto & port = *static_cast<MidiPort*>(client);

	const MIDIPacket * packet = &packetlist->packet[0];

	REFLEX_LOOP(x, packetlist->numPackets)
	{
		const UInt8 * data = packet->data;
		
		UInt offset = 0;

		UInt8 running_status = 0;

		// CoreMIDI packets may contain a stream of mixed MIDI messages, not a single
		// repeated message type for the whole packet.
		while (offset < packet->length)
		{
			UInt8 status = data[offset];

			if (status >= 0x80)
			{
				if (status < 0xF0)
				{
					running_status = status;

					if (UInt size = kMidiTypeToSize[(status >> 4) & 15])
					{
						if (offset + size > packet->length) break;

						QueueShortMessage(self, port, data + offset, size);

						offset += size;

						continue;
					}
				}
				else if (status >= 248 && status <= 252)	//clock/start/continue/stop
				{
					QueueShortMessage(self, port, data + offset, 1);

					offset += 1;

					continue;
				}
				else
				{
					switch (status)
					{
						case 0xF0:	//sysex start
							running_status = 0;
							offset += 1;
							while (offset < packet->length)
							{
								UInt8 sysex_byte = data[offset];

								if (sysex_byte == 0xF7)
								{
									offset += 1;

									break;
								}

								if (sysex_byte >= 248 && sysex_byte <= 252)
								{
									QueueShortMessage(self, port, data + offset, 1);
								}

								offset += 1;
							}
							continue;

						case 0xF1:
						case 0xF3:
							running_status = 0;
							offset += Min<UInt>(2, packet->length - offset);
							continue;

						case 0xF2:
							running_status = 0;
							offset += Min<UInt>(3, packet->length - offset);
							continue;

						default:
							running_status = 0;
							offset += 1;
							continue;
					}
				}
			}
			else if (running_status)
			{
				if (UInt size = kMidiTypeToSize[(running_status >> 4) & 15])
				{
					UInt payload_size = size - 1;

					if (offset + payload_size > packet->length) break;

					UInt8 running_message[3] =
					{
						running_status,
						data[offset],
						payload_size > 1 ? data[offset + 1] : UInt8(0)
					};

					QueueShortMessage(self, port, running_message, size);

					offset += payload_size;

					continue;
				}
			}

			offset += 1;
		}

		packet = MIDIPacketNext(packet);
	}
}

void CoreAudio::GetMidiPortName(const MIDIEndpointRef & midiendpointref, UInt32 idx, WString & string)
{
	char buffer[128] = {0};

	CFStringRef cfstringref;

	MIDIObjectGetStringProperty(midiendpointref, kMIDIPropertyModel, &cfstringref);

	if (cfstringref != NULL)
	{
		CFStringGetCString(cfstringref, buffer, sizeof(buffer), 0);

		string = ToWString(Join(buffer, " "));
	}
	else
	{
		string = Join(L"Port ", Reflex::ToWString(idx), L" ");
	}

	MIDIObjectGetStringProperty(midiendpointref, kMIDIPropertyName, &cfstringref);

	if (cfstringref != NULL)
	{
		CFStringGetCString(cfstringref, buffer, sizeof(buffer), 0);

		string = Join(string, ToWString(CString::View(buffer + 0)));
	}
}

void CoreAudio::EnumerateAudioDevices(void * client, FunctionPointer<bool(void*,const AudioDeviceInfo &)> callback)
{
	//get num device

	AudioObjectPropertyAddress adr = { kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };

	::UInt32 size = 0;

	AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &adr, 0, NULL, &size);

	UInt32 ndevice = size / sizeof(AudioDeviceID);


	//get device ids

	Array <AudioDeviceID> deviceids(ndevice);

	AudioObjectGetPropertyData(kAudioObjectSystemObject, &adr, 0, NULL, &size, deviceids.GetData());


	char string[256] = {0};

	AudioDeviceID query = kAudioObjectUnknown;

	AudioDeviceInfo info;

	for (auto & deviceid : deviceids)
	{
		info.deviceid = deviceid;


		adr.mSelector = kAudioDevicePropertyDeviceName;

		size = 256;

		string[0] = {0};

		AudioObjectGetPropertyData(deviceid, &adr, 0, 0, &size, &string);

		info.name = ToWString(CString::View(string + 0));


		adr.mSelector = kAudioHardwarePropertyDefaultOutputDevice;

		size = kSizeOf<AudioDeviceID>;

		query = kAudioObjectUnknown;

		AudioObjectGetPropertyData(kAudioObjectSystemObject, &adr, 0, nullptr, &size, &query);

		info.default_output = deviceid == query;

		if (!callback(client, info)) break;
	}
}

OSStatus CoreAudio::ProcessAudioCallbackRt(AudioDeviceID device, const AudioTimeStamp * inNow, const AudioBufferList * input, const AudioTimeStamp * inputtime, AudioBufferList * output, const AudioTimeStamp * outputtime, void * client)
{
	static_cast<CoreAudio*>(client)->ProcessRt(input, output);

	return noErr;
}

REFLEX_INLINE void CoreAudio::ProcessRt(const AudioBufferList * input, AudioBufferList * output)
{
	ProcessingScopeRt scope(this);
	
	if (scope.IsLocked()) return;


	//audio in

	auto input_channels = m_channels[0].GetData();

	REFLEX_LOOP(idx, input->mNumberBuffers)
	{
		auto & audiobuffer = input->mBuffers[idx];

		UInt32 nchn = audiobuffer.mNumberChannels;

		(*kProcessInputFns[Min<UInt32>(nchn, 3)])(audiobuffer, input_channels, m_buffersize);

		input_channels += nchn;
	}



	//process

	/*auto eventsout = */ Common::DesktopAudioAppBase::ProcessRt(scope, m_buffersize);



	//audio out

	auto output_channels = m_channels[1].GetData();

	REFLEX_LOOP(idx, output->mNumberBuffers)
	{
		auto & audiobuffer = output->mBuffers[idx];

		UInt32 nchn = audiobuffer.mNumberChannels;

		(*kProcessOutputFns[Min<UInt32>(nchn, 17)])(output_channels, audiobuffer, m_buffersize);

		output_channels += nchn;
	}



	//TODO midi out

	//MIDISend
}

void CoreAudio::ProcessMonoInputRt(const AudioBuffer & audiobuffer, Channel * channels, UInt buffersize)
{
	Float32 * client_in0 = (*channels++).buffer;

	const Float32 * coreaudio_in = Reinterpret<Float32>(audiobuffer.mData);

	for (UInt32 sample = 0; sample < buffersize; sample += 8)
	{
		*client_in0++ = *coreaudio_in++;
		*client_in0++ = *coreaudio_in++;
		*client_in0++ = *coreaudio_in++;
		*client_in0++ = *coreaudio_in++;
		*client_in0++ = *coreaudio_in++;
		*client_in0++ = *coreaudio_in++;
		*client_in0++ = *coreaudio_in++;
		*client_in0++ = *coreaudio_in++;
	}
}

void CoreAudio::ProcessStereoInputRt(const AudioBuffer & audiobuffer, Channel * channels, UInt buffersize)
{
	Float32 * client_in0 = (*channels++).buffer;

	Float32 * client_in1 = (*channels++).buffer;

	const Float32 * coreaudio_in = Reinterpret<Float32>(audiobuffer.mData);

	for (UInt32 sample = 0; sample < buffersize; sample += 8)
	{
		*client_in0++ = *coreaudio_in++;
		*client_in1++ = *coreaudio_in++;

		*client_in0++ = *coreaudio_in++;
		*client_in1++ = *coreaudio_in++;

		*client_in0++ = *coreaudio_in++;
		*client_in1++ = *coreaudio_in++;

		*client_in0++ = *coreaudio_in++;
		*client_in1++ = *coreaudio_in++;

		*client_in0++ = *coreaudio_in++;
		*client_in1++ = *coreaudio_in++;

		*client_in0++ = *coreaudio_in++;
		*client_in1++ = *coreaudio_in++;

		*client_in0++ = *coreaudio_in++;
		*client_in1++ = *coreaudio_in++;

		*client_in0++ = *coreaudio_in++;
		*client_in1++ = *coreaudio_in++;
	}
}

void CoreAudio::ProcessInputsRt(const AudioBuffer & audiobuffer, Channel * channels, UInt buffersize)
{
	UInt32 nchn = audiobuffer.mNumberChannels;

	Float32 * buffers[nchn];

	REFLEX_LOOP(idx, nchn) buffers[idx] = (*channels++).buffer;

	const Float32 * coreaudio_in = Reinterpret<Float32>(audiobuffer.mData);

	REFLEX_LOOP(sample, buffersize)
	{
		REFLEX_LOOP(channel, nchn) *buffers[channel]++ = *coreaudio_in++;
	}
}

void CoreAudio::ProcessMonoOutputRt(const Channel * psource, AudioBuffer & audiobuffer, UInt buffersize)
{
	const Float32 * client_out = psource->buffer;

	Float32 * coreaudio_out = Reinterpret<Float32>(audiobuffer.mData);

	for (UInt32 sample = 0; sample < buffersize; sample += 8)
	{
		(*coreaudio_out++) = (*client_out++);
		(*coreaudio_out++) = (*client_out++);
		(*coreaudio_out++) = (*client_out++);
		(*coreaudio_out++) = (*client_out++);
		(*coreaudio_out++) = (*client_out++);
		(*coreaudio_out++) = (*client_out++);
		(*coreaudio_out++) = (*client_out++);
		(*coreaudio_out++) = (*client_out++);
	}
}

void CoreAudio::ProcessStereoOutputRt(const Channel * psource, AudioBuffer & audiobuffer, UInt buffersize)
{
	const Float32 * client_out0 = psource[0].buffer;
	const Float32 * client_out1 = psource[1].buffer;

	Float32 * coreaudio_out = Reinterpret<Float32>(audiobuffer.mData);

	for (UInt32 sample = 0; sample < buffersize; sample += 8)
	{
		(*coreaudio_out++) = (*client_out0++);
		(*coreaudio_out++) = (*client_out1++);

		(*coreaudio_out++) = (*client_out0++);
		(*coreaudio_out++) = (*client_out1++);

		(*coreaudio_out++) = (*client_out0++);
		(*coreaudio_out++) = (*client_out1++);

		(*coreaudio_out++) = (*client_out0++);
		(*coreaudio_out++) = (*client_out1++);

		(*coreaudio_out++) = (*client_out0++);
		(*coreaudio_out++) = (*client_out1++);

		(*coreaudio_out++) = (*client_out0++);
		(*coreaudio_out++) = (*client_out1++);

		(*coreaudio_out++) = (*client_out0++);
		(*coreaudio_out++) = (*client_out1++);

		(*coreaudio_out++) = (*client_out0++);
		(*coreaudio_out++) = (*client_out1++);
	}
}

template <Reflex::UInt32 N> void CoreAudio::ProcessOutputsRt(const Channel * channels, AudioBuffer & audiobuffer, UInt buffersize)
{
	Float32 * coreaudio_out = Reinterpret<Float32>(audiobuffer.mData);

	if (N)
	{
		const Float32 * sources[N + 1];

		REFLEX_LOOP(idx, N) sources[idx] = (*channels++).buffer;

		REFLEX_LOOP(sample, buffersize)
		{
			REFLEX_LOOP(chn, N)
			{
				auto value = (*sources[chn]++);

				(*coreaudio_out++) = value;
			}
		}
	}
	else
	{
		UInt32 nchn = audiobuffer.mNumberChannels;

		const Float32 * sources[nchn];

		REFLEX_LOOP(idx, nchn) sources[idx] = (*channels++).buffer;

		REFLEX_LOOP(sample, buffersize)
		{
			REFLEX_LOOP(chn, nchn)
			{
				(*coreaudio_out++) = (*sources[chn]++);
			}
		}
	}
}

#if	(REFLEX_DEBUG)
AudioObjectPropertyScope GetAudioObjectPropertyScope(AudioDevicePropertyID property_id, Boolean is_input)
{
	switch (property_id)
	{
		case kAudioDevicePropertyStreamConfiguration:
			return is_input ? kAudioObjectPropertyScopeInput : kAudioObjectPropertyScopeOutput;

		default:
			return kAudioObjectPropertyScopeGlobal;
	}
}
#endif

OSStatus CoreAudio::AudioDeviceGetPropertyInfo(AudioDeviceID device, UInt32 chn, Boolean is_input, AudioDevicePropertyID property_id, UInt32 * size_out, Boolean * writeable_out)
{
#if	REFLEX_DEBUG
	AudioObjectPropertyAddress addr =
	{
		(AudioObjectPropertySelector)property_id,
		GetAudioObjectPropertyScope(property_id, is_input),
		chn ? (AudioObjectPropertyElement)chn : kAudioObjectPropertyElementMain
	};

	UInt32 size = 0;
	OSStatus err = AudioObjectGetPropertyDataSize(device, &addr, 0, nullptr, &size);

	if (size_out)
	{
		*size_out = (err == noErr) ? size : 0;
	}

	if (writeable_out)
	{
		Boolean settable = false;

		if (err == noErr)
		{
			OSStatus err2 = AudioObjectIsPropertySettable(device, &addr, &settable);

			// If this query fails, behave conservatively.
			if (err2 != noErr) settable = false;
		}

		*writeable_out = settable;
	}

	return err;
#else
	return ::AudioDeviceGetPropertyInfo(device, chn, is_input, property_id, size_out, writeable_out);
#endif
}

OSStatus CoreAudio::AudioDeviceGetProperty(AudioDeviceID device, UInt32 chn, Boolean is_input, AudioDevicePropertyID property_id, UInt32 * io_size, void * out_data)
{
	if (!io_size || (!out_data && *io_size))
	{
		return kAudio_ParamError;
	}

#if	REFLEX_DEBUG
	AudioObjectPropertyAddress addr =
	{
		(AudioObjectPropertySelector)property_id,
		GetAudioObjectPropertyScope(property_id, is_input),
		chn ? (AudioObjectPropertyElement)chn : kAudioObjectPropertyElementMain
	};

	// qualifier not supported by legacy call signature, so pass none
	return AudioObjectGetPropertyData(device, &addr, 0, nullptr, io_size, out_data);
#else
	return ::AudioDeviceGetProperty(device, chn, is_input, property_id, io_size, out_data);
#endif
}

OSStatus CoreAudio::AudioDeviceSetProperty(AudioDeviceID device, int unused, UInt32 chn, Boolean is_input, AudioDevicePropertyID property_id, UInt32 data_size, const void * data)
{
	REFLEX_ASSERT(unused == 0);

	if ((data_size != 0) && !data)
	{
		return kAudio_ParamError;
	}

#if	REFLEX_DEBUG
	AudioObjectPropertyAddress addr =
	{
		(AudioObjectPropertySelector)property_id,
		GetAudioObjectPropertyScope(property_id, is_input),
		chn ? (AudioObjectPropertyElement)chn : kAudioObjectPropertyElementMain
	};

	return AudioObjectSetPropertyData(device, &addr, 0, nullptr, data_size, data);
#else
	return ::AudioDeviceSetProperty(device, nullptr, chn, is_input, property_id, data_size, data);
#endif
}

CoreAudio * CoreAudio::st_self;

const Float64 CoreAudio::kSampleRates[5] = {44100, 48000, 88200, 96000, 192000};

const FunctionPointer <void(const AudioBuffer&,CoreAudio::Channel*,UInt32)> CoreAudio::kProcessInputFns[] =
{
	&CoreAudio::ProcessNullInputRt,
	&CoreAudio::ProcessMonoInputRt,
	&CoreAudio::ProcessStereoInputRt,
	&CoreAudio::ProcessInputsRt,
};

const FunctionPointer <void(const CoreAudio::Channel*,AudioBuffer&,UInt32)> CoreAudio::kProcessOutputFns[] =
{
	&CoreAudio::ProcessNullOutputRt,
	&CoreAudio::ProcessMonoOutputRt,
	&CoreAudio::ProcessStereoOutputRt,
	&CoreAudio::ProcessOutputsRt<3>,
	&CoreAudio::ProcessOutputsRt<4>,
	&CoreAudio::ProcessOutputsRt<5>,
	&CoreAudio::ProcessOutputsRt<6>,
	&CoreAudio::ProcessOutputsRt<7>,
	&CoreAudio::ProcessOutputsRt<8>,
	&CoreAudio::ProcessOutputsRt<9>,
	&CoreAudio::ProcessOutputsRt<10>,
	&CoreAudio::ProcessOutputsRt<11>,
	&CoreAudio::ProcessOutputsRt<12>,
	&CoreAudio::ProcessOutputsRt<13>,
	&CoreAudio::ProcessOutputsRt<14>,
	&CoreAudio::ProcessOutputsRt<15>,
	&CoreAudio::ProcessOutputsRt<16>,

	&CoreAudio::ProcessOutputsRt<0>,
};

const UInt32 CoreAudio::kMidiTypeToSize[16] =
{
	0,
	0,
	0,
	0,

	0,
	0,
	0,
	0,

	3,
	3,
	3,
	3,

	2,
	2,
	3,

	0
};

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::Common::DesktopAudioAppBase> Reflex::System::OSX::CreateCoreAudio()
{
	return REFLEX_CREATE(OSX::CoreAudio);
}

Reflex::WString Reflex::System::AudioPlugin::GetDefaultDevice(bool)
{
	WString default_device;

	OSX::CoreAudio::EnumerateAudioDevices(&default_device, [](void * client, const OSX::CoreAudio::AudioDeviceInfo & desc)
	{
		if (desc.default_output)
		{
			*Cast<WString>(client) = desc.name;

			return false;
		}
		else
		{
			return true;
		}
	});

	return default_device;
}

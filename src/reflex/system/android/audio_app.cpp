#include "audio_app.h"
#include "jni/[require].h"
#include "jni/android_audio.hpp"
#include "../common/simd_utils.h"
#include <oboe/Oboe.h>
#include <aaudio/AAudio.h>

/// MEMO: Takes time (to test all the streams), so we've disabled it for the moment.
#define ENUMERATE_SAMPLERATES_AND_BUFFERSIZES 0

REFLEX_NS(Reflex::System::Android)
/**
 * MEMO: Android supports mix-and-matching devices (ex. input from an external mic, output on the earphones), unlike iOS. Reflex should expose something like that, since desktops support it too. Branch android-audio-mix-match-channels has a stub of this work.
 *
 * MEMO: starting an input stream in oboe does not trigger the BT for telephony codec.
 *
 * MEMO: see this about selecting the optimal sample rate: https://github.com/google/oboe/issues/526
 */
struct AudioApp : public Common::MobileAudioAppBase, public oboe::AudioStreamCallback {
	static constexpr WString::View kDefaultDevice = L"Android audio";

	// lifetime
	AudioApp();
	~AudioApp() override;

	void OnAudioDevicesChanged();

	using Standalone::OpenEditor;
	using Standalone::CloseEditor;

private:

	static constexpr Float kTimeToDrain           = 0.2f;
	static constexpr Float kTimeToDiscard         = 0.3f;
	static constexpr UInt  kNumInputBurstsCushion = 0; // let input fill back up, usually 0 or 1
	static constexpr UInt  kNumCallbacksToDrain   = 20;
	static constexpr UInt  kNumCallbacksToDiscard = 30;

	enum Direction {
		kDirectionInput = 0,
		kDirectionOutput = 1,
		kNumDirection,
	};

	enum PlaybackState {
		kPlaybackStateStopped = 0,
		kPlaybackStateStarting = 1,
		kPlaybackStateRunning = 2,
	};

	/**
	 * Example: phone having a speaker, a back and a front microphone, and a connected headset.
	 * DeviceDescriptor: ["speaker", "headset"]
	 * ChannelDescriptor: "speaker" -> ["back mic", "front mic"], "headset" -> ["microphone"]
	 * Channel (if "speaker" device connected): ["back mic ch.1", "back mic ch.2", "front mic ch.1"]
	 * StreamingChannel (if the second back mic channel has been selected):
	 * 		[empty buffer for unselected first mic channel, buffer for second mic channel]
	 * 		— 2 channels since the "back mic" stream is stereo: if the second channel has been
	 * 		selected, we want to take the 2nd channel from the hardware buffer.
	 */
	struct ChannelDescriptor {
		WString friendly_name;
		int device_id;
		int channel_count;
		bool is_output;
		bool is_default;
	};

	struct DeviceDescriptor {
		Array<float> sample_rates;
		Array<ChannelDescriptor> channels[kNumDirection];
	};

	struct Channel {
		int device_id;
		int sibling_count;
		bool enabled;
		WString name;
	};

	// AudioApp specialization
	Array<WString> GetAvailableAudioDevices() const override;
	bool OnSelectAudioDevice(const WString::View& value) override;
	WString GetCurrentAudioDevice() const override { return m_currentdevice; }

	Array<UInt32> GetAvailableBufferSizes() const override;
	void OnRequestBufferSize(UInt32 value) override;
	UInt32 GetCurrentBufferSize() const override { return m_buffersize; }

	Array<Float32> GetAvailableSampleRates() const override;
	void OnRequestSampleRate(Float32 value) override;
	Float32 GetCurrentSampleRate() const override { return m_samplerate; }

	Array<Pair<WString,bool>> GetAudioChannels(bool output) const override;
	void OnEnableAudioChannel(bool output, UInt32 idx, bool enable) override;

	// TODO: Florian -- MIDI
	Array<Pair<WString,bool>> GetMidiPorts(bool output) const override { return {}; }
	void OnEnableMidiPort(bool output, UInt idx, bool enable) override {}

	void OnPause() override;
	void OnResume() override;

	// oboe::AudioStreamCallback specialization
	oboe::DataCallbackResult onAudioReady(oboe::AudioStream* out_stream, void* out_audio, int32_t num_frames) override;
	void onErrorAfterClose(oboe::AudioStream* stream, oboe::Result result) override;

	// Internal management
	void CollectDevices(bool preserveChannelStates);
	void DisableDeviceChannels(int device_id);
	void OnAudioDevicesChangedInternal();
	bool SetAudioDevice(const WString::View& device_name, bool preserveChannelStates);

	void StartAudio();
	void StopAudio();
	void StopAudioAbnormal(const oboe::AudioStream* failing_stream);

	void UpdateBuffers(UInt buffersize);

	bool CheckStreamFormat(int direction, oboe::AudioStream* stream, UInt32 num_frames);
	void ReadInputInterleaved(oboe::AudioStream* stream, void* audio_data, UInt32 buffersize);
	void WriteOutputSilence(oboe::AudioStream* stream, void* audio_data, UInt num_samples);
	void WriteOutputInterleaved(oboe::AudioStream* stream, void* audio_data, UInt32 buffersize);

	Channel* GetAnyEnabledChannel(int direction);
	void BuildStream(const Channel& c, oboe::AudioStreamBuilder& builder) const;
	static bool OpenTestStream(int direction, int deviceid, Float32 samplerate, UInt32 buffersize, std::shared_ptr<oboe::AudioStream>& out_stream);

	Map<WString,DeviceDescriptor> m_availabledevices;

	WString m_currentdevice;
	Array<Float32> m_availablesamplerates;
	Array<UInt32> m_availablebuffersizes;
	Array<Channel> m_availablechannels[kNumDirection];
	Array<Array<Float32>> m_streamingchannels[kNumDirection];
	std::shared_ptr<oboe::AudioStream> m_stream[kNumDirection];

	Array<Float32> m_silence_buffer;
	UInt32 m_buffersize = kDefaultBufferSize;
	Float32 m_samplerate = kDefaultSampleRate;
	std::atomic<PlaybackState> m_playbackstate = kPlaybackStateStopped;
	// Let the input fill up a bit so we are not so close to the write pointer.
	UInt32 m_callbackstocushion = 0;
	// Drain the input (at first, we might have a lot of data queued on the input).
	UInt32 m_callbackstodrain = 0;
	// Ignore. Allow the input to reach to equilibrium with the output.
	UInt32 m_callbackstodiscard = 0;

	Reflex::Detail::ThreadValidator <REFLEX_DEBUG> m_threadvalidator;

	//Debug::Profiler m_profiler_processrt, m_profiler_wholeoutput;
};

AudioApp::AudioApp()
	// : m_profiler_processrt(Common::log, "ProcessRt")
	// , m_profiler_wholeoutput(Common::log, "onAudioReadyForOutput")
{
	CollectDevices(false);
}

AudioApp::~AudioApp() {
	REFLEX_USE(Jni)

	StopAudio();
	g_reflexActivityInstance->EnableBackgroundAudioService(AttachedJavaEnv(), false);
}

void AudioApp::OnAudioDevicesChanged() {
	Lock lock(*this);
	OnAudioDevicesChangedInternal();
}

void AudioApp::OnAudioDevicesChangedInternal() {
	CollectDevices(true);
	globals->m_signals[kNotificationChangeDevices].Notify();
}

bool AudioApp::SetAudioDevice(const Array<Reflex::WChar>::View& device_name, bool preserveChannelStates) {
	REFLEX_ASSERT(m_threadvalidator)

	Map<WString, bool> channelsState[kNumDirection];
	m_availablesamplerates.Clear();
	m_availablebuffersizes.Clear();
	REFLEX_LOOP(direction, kNumDirection) {
		if (preserveChannelStates) {
			REFLEX_FOREACH(ch, m_availablechannels[direction]) {
				channelsState[direction].Set(ch.name, ch.enabled);
			}
		}
		m_availablechannels[direction].Clear();
		m_streamingchannels[direction].Clear();
	}

	if (auto pdevice = m_availabledevices.Search(device_name)) {
		if (m_currentdevice != device_name) {
			m_currentdevice = device_name;
		}

		m_samplerate = kDefaultSampleRate;
		m_buffersize = kDefaultBufferSize;

		// Collect channels
		REFLEX_LOOP(direction, kNumDirection) {
			REFLEX_FOREACH(device_channel, pdevice->channels[direction]) {
				REFLEX_LOOP(chidx, device_channel.channel_count) {
					// Do not duplicate channel names
					WString name (Join(device_channel.friendly_name, L" Ch.", Reflex::ToWString(chidx + 1)));
					Channel* to_replace = nullptr;
					for (Channel& c : m_availablechannels[direction]) {
						if (c.name == name) {
							to_replace = &c;
							break;
						}
					}

					Channel desc = {
						.device_id = device_channel.device_id,
						.sibling_count = device_channel.channel_count,
						.enabled = channelsState[direction].Acquire(name, false),
						.name = name
					};
					if (to_replace) {
						*to_replace = desc;
					}
					else {
						m_availablechannels[direction].Push(desc);
					}
				}
			}
		}

#if ENUMERATE_SAMPLERATES_AND_BUFFERSIZES
		// And sample rates / buffer sizes
	std::shared_ptr<oboe::AudioStream> stream;
	int direction = device.channel_count[kDirectionOutput] > 0 ? kDirectionOutput : kDirectionInput;
	REFLEX_FOREACH(sr, device.sample_rates) {
		if (OpenTestStream(direction, device.device_id[kDirectionOutput], sr, oboe::kUnspecified, stream)) {
			auto actual_sr = stream->getSampleRate();
			if (IsAlmostEqual(Float32(actual_sr), sr)) {
				m_availablesamplerates.Push(sr);
			}
			stream->close();
		}
	}

	REFLEX_FOREACH(bs, kStandardBufferSizes) {
		if (OpenTestStream(direction, device.device_id[kDirectionOutput], oboe::kUnspecified, bs, stream)) {
			auto actual_bs = stream->getBufferCapacityInFrames();
			if (actual_bs == bs) {
				m_availablebuffersizes.Push(bs);
			}
			stream->close();
		}
	}
#else
		m_availablesamplerates = pdevice->sample_rates;
		m_availablebuffersizes = ToView(kStandardBufferSizes);
#endif
		return true;
	}

	return false;
}

Array<WString> AudioApp::GetAvailableAudioDevices() const {
	Array<WString> result;
	REFLEX_FOREACH(d, m_availabledevices) result.Push(d.key);
	return result;
}

bool AudioApp::OnSelectAudioDevice(const WString::View& value) {
	return SetAudioDevice(value, false);
}

Array<UInt32> AudioApp::GetAvailableBufferSizes() const {
	return ToView(kStandardBufferSizes);
}

Array<Float32> AudioApp::GetAvailableSampleRates() const {
	return m_availablesamplerates;
}

void AudioApp::OnRequestBufferSize(UInt32 value) {
	m_buffersize = value;
}

void AudioApp::OnRequestSampleRate(Float32 value) {
	m_samplerate = value;
}

Array<Pair<WString,bool>> AudioApp::GetAudioChannels(bool output) const {
	Array<Pair<WString,bool>> result;
	REFLEX_FOREACH(channel, m_availablechannels[output]) {
		result.Push({ channel.name, channel.enabled });
	}
	return result;
}

void AudioApp::OnEnableAudioChannel(bool output, UInt32 idx, bool enable) {
	auto* already_enabled = GetAnyEnabledChannel(output);
	if (already_enabled && already_enabled->device_id != m_availablechannels[output][idx].device_id) {
		DEV_LOG("AudioApp::Unsupported selection of",
				output ? "output" : "input",
				"channels from different devices at the same time");
		already_enabled->enabled = false;
	}

	m_availablechannels[output][idx].enabled = enable;
}

void AudioApp::OnPause() {
	StopAudio();
}

void AudioApp::OnResume() {
	StartAudio();
}

//
// -------------------------------- Private -------------------------------- //
//
void AudioApp::CollectDevices(bool preserveChannelStates) {
	REFLEX_USE(Jni)

	REFLEX_ASSERT(m_threadvalidator)
	m_availabledevices.Clear();

	AttachedJavaEnv env;
	auto devices = g_reflexActivityInstance->GetAudioDevices(env, kDefaultDevice);

	REFLEX_FOREACH(device, devices) {
		auto dir = device->isInput ? kDirectionInput : kDirectionOutput;
		auto& desc = m_availabledevices.Acquire(ToWString(device->productName));

		// A device with the same name might appear multiple times
		desc.channels[dir].Push(ChannelDescriptor {
			.friendly_name = ToWString(device->friendlyName),
			.device_id = device->id,
			/// MEMO: this may not be the best way to enumerate channels. For example, the Pixel 9 Pro reports supporting 4 channels for the main microphone. When opening an oboe stream without calling setChannelCount on the builder will open a 2-channel stream instead, which makes more sense. Since opening a stream is a slow operation and providing a list of devices quickly has value, we have left it as is.
			.channel_count = device->numChannels,
			.is_output = !device->isInput,
			.is_default = device->isDefault
		});

		if (desc.sample_rates.Empty()) {
			desc.sample_rates = device->sampleRates;
		}
		else {
			Intersect(desc.sample_rates, device->sampleRates);
		}
	}

	SetAudioDevice(m_currentdevice, preserveChannelStates);
}

void AudioApp::DisableDeviceChannels(int device_id) {
	REFLEX_LOOP(direction, kNumDirection) {
		REFLEX_FOREACH(ch, m_availablechannels[direction]) {
			if (ch.device_id == device_id) {
				ch.enabled = false;
			}
		}
	}
}

void AudioApp::StartAudio() {
	REFLEX_USE(Jni)

	REFLEX_ASSERT(m_threadvalidator && m_playbackstate == kPlaybackStateStopped)
	if (!m_currentdevice) return;

	Channel* in_channel = GetAnyEnabledChannel(kDirectionInput);
	Channel* out_channel = GetAnyEnabledChannel(kDirectionOutput);
	if (!in_channel && !out_channel) return;

	auto desired_sharingmode = oboe::SharingMode::Shared;
	auto actual_sr = Float(oboe::kUnspecified);
	auto actual_bs = UInt(oboe::kUnspecified);
	Jni::AttachedJavaEnv env;
	oboe::Result result;

	m_callbackstocushion = kNumInputBurstsCushion;
	m_callbackstodiscard = Min(kNumCallbacksToDiscard, UInt32(RoundNearest(kTimeToDiscard * m_samplerate / Float(m_buffersize))));
	m_callbackstodrain =  Min(kNumCallbacksToDrain, UInt32(RoundNearest(kTimeToDrain * m_samplerate / Float(m_buffersize))));
	m_playbackstate = kPlaybackStateStarting;

	if (in_channel) {
		desired_sharingmode = oboe::SharingMode::Exclusive;

		if (!g_reflexActivityInstance->HasAlreadyGotRecordingPermission()
			&& !g_reflexActivityInstance->RequestRecordingPermission(env)) {
			StopAudio();
			return;
		}

		oboe::AudioStreamBuilder builder;
		BuildStream(*in_channel, builder);
		builder.setDirection(oboe::Direction::Input);
		builder.setSharingMode(desired_sharingmode);
		if (!out_channel) {
			// As per FullDuplexStream (an sample from the official oboe repo) shows, we have to set a callback on the output stream only (but if we have only one input, then we have to set it there).
			builder.setCallback(this);
		}

		// FIXME: [Florian] take note of the fact that audio does not work when the app is in the background
		auto success = (result = builder.openStream(m_stream[kDirectionInput])) == oboe::Result::OK
					   && (result = m_stream[kDirectionInput]->requestStart()) == oboe::Result::OK;
		if (!success) {
			DEV_WARN("Failed to open input stream", oboe::convertToText(result));
			StopAudio();
			return;
		}

		actual_sr = Float(m_stream[kDirectionInput]->getSampleRate());
		actual_bs = UInt(m_stream[kDirectionInput]->getFramesPerDataCallback());
	}

	if (out_channel) {
		oboe::AudioStreamBuilder builder;
		BuildStream(*out_channel, builder);
		builder.setDirection(oboe::Direction::Output);
		builder.setSharingMode(desired_sharingmode);
		builder.setCallback(this);

		auto success = (result = builder.openStream(m_stream[kDirectionOutput])) == oboe::Result::OK
					   && (result = m_stream[kDirectionOutput]->requestStart()) == oboe::Result::OK;
		if (!success) {
			DEV_WARN("Failed to open output stream", oboe::convertToText(result));
			StopAudio();
			return;
		}

		actual_sr = Float(m_stream[kDirectionOutput]->getSampleRate());
		actual_bs = UInt(m_stream[kDirectionOutput]->getFramesPerDataCallback());
	}

	if (actual_sr != oboe::kUnspecified) m_samplerate = actual_sr;
	if (actual_bs != oboe::kUnspecified) m_buffersize = actual_bs;

	UpdateBuffers(m_buffersize);
	PrepareProcessing(m_buffersize, m_samplerate);

	m_playbackstate = kPlaybackStateRunning;

	g_reflexActivityInstance->EnableBackgroundAudioService(env, true);
}

void AudioApp::StopAudio() {
	REFLEX_USE(Jni)

	REFLEX_ASSERT(m_threadvalidator)

	Jni::AttachedJavaEnv env;

	m_playbackstate = kPlaybackStateStopped;
	REFLEX_FOREACH(stream_ptr, m_stream) {
		if (stream_ptr) {
			stream_ptr->close();
			stream_ptr = nullptr;
		}
	}

	g_reflexActivityInstance->EnableBackgroundAudioService(env, false);
}

void AudioApp::StopAudioAbnormal(const oboe::AudioStream* failing_stream) {
	m_playbackstate = kPlaybackStateStopped;

	// Try to restart the stream
	globals->PostToReflexThread("RestartAudio", [this] {
		StopAudio();
		StartAudio();
	});
}

void AudioApp::UpdateBuffers(UInt buffersize) {
	REFLEX_ASSERT(buffersize > 0)

	ClearBuffers();
	m_silence_buffer.SetSize(buffersize);
	m_silence_buffer.Wipe();

	REFLEX_LOOP(direction, kNumDirection) {
		auto& channels = m_availablechannels[direction];
		auto& buffers = m_streamingchannels[direction];
		int device_id = -1;
		UInt nchannel = 0;

		REFLEX_FOREACH(ch, channels) {
			if (ch.enabled) {
				device_id = ch.device_id;
				nchannel += 1;
			}
		}

		buffers.Clear();
		REFLEX_FOREACH(ch, channels) {
			if (ch.device_id != device_id) continue;

			auto& buffer = buffers.Push();
			if (ch.enabled) {
				buffer.SetSize(buffersize);
				buffer.Wipe();
				AddBuffer(direction, buffer.GetData());
			}
		}
	}
}

bool AudioApp::CheckStreamFormat(int direction, oboe::AudioStream* stream, UInt32 num_frames) {
	if (stream->getFormat() != oboe::AudioFormat::Float) {
		DEV_WARN("AudioApp::Mismatch in stream format", int(stream->getFormat()));
		return false;
	}

	if (stream->getChannelCount() != m_streamingchannels[direction].GetSize()) {
		DEV_WARN("AudioApp::Mismatch in stream channels", stream->getChannelCount(), "vs", m_streamingchannels[direction].GetSize());
		return false;
	}

	if (num_frames != m_buffersize) {
		DEV_WARN("AudioApp::Mismatch in stream buffer size", num_frames, "vs", m_buffersize);
		return false;
	}

	return true;
}

void AudioApp::ReadInputInterleaved(oboe::AudioStream* stream, void* audio_data, UInt32 buffersize) {
	auto& streamingch = m_streamingchannels[kDirectionInput];
	UInt32 nchn = streamingch.GetSize();
	REFLEX_ASSERT(stream->getFormat() == oboe::AudioFormat::Float && nchn == stream->getChannelCount())

	Float32* buffers[nchn];
	Float32 dummybuffer[buffersize];
	const Float32* audio_in = Reinterpret<Float32>(audio_data);

	REFLEX_LOOP(idx, nchn) {
		buffers[idx] = streamingch[idx].Empty() ? dummybuffer : streamingch[idx].GetData();
	}

	REFLEX_LOOP(sample, buffersize) {
		REFLEX_LOOP(channel, nchn) *(buffers[channel]++) = (*audio_in++);
	}
}

void AudioApp::WriteOutputSilence(oboe::AudioStream* stream, void* audio_data, UInt num_samples) {
	MemClear(audio_data, num_samples * stream->getBytesPerFrame());
}

void AudioApp::WriteOutputInterleaved(oboe::AudioStream* stream, void* audio_data, UInt32 buffersize) {
	auto& streamingch = m_streamingchannels[kDirectionOutput];
	UInt32 nchn = streamingch.GetSize();
	REFLEX_ASSERT(stream->getFormat() == oboe::AudioFormat::Float && nchn == stream->getChannelCount())

	const Float32* sources[nchn];
	Float32* audio_out = Reinterpret<Float32>(audio_data);

	REFLEX_LOOP(idx, nchn) {
		sources[idx] = streamingch[idx].Empty() ? m_silence_buffer.GetData() : streamingch[idx].GetData();
	}

	REFLEX_LOOP(sample, buffersize) {
		REFLEX_LOOP(chn, nchn) (*audio_out++) = (*sources[chn]++);
	}
}

oboe::DataCallbackResult AudioApp::onAudioReady(oboe::AudioStream* stream, void* out_audio, int32_t num_frames) {
	if (m_playbackstate != kPlaybackStateRunning) {
		WriteOutputSilence(stream, out_audio, num_frames);
		return m_playbackstate == kPlaybackStateStopped
			? oboe::DataCallbackResult::Stop
			: oboe::DataCallbackResult::Continue;
	}

	ProcessingScopeRt scope(this);
	if (scope.IsLocked()) {
		DEV_WARN("AudioApp::Could not acquire ProcessRt scope");
		WriteOutputSilence(stream, out_audio, num_frames);
		return oboe::DataCallbackResult::Continue;
	}

	if (auto* in_stream = m_stream[kDirectionInput].get()) {
		if (!CheckStreamFormat(kDirectionInput, in_stream, num_frames)) {
			WriteOutputSilence(stream, out_audio, num_frames);
			StopAudioAbnormal(in_stream);
			return oboe::DataCallbackResult::Stop;
		}

		bool audio_read = false;
		int actualFramesRead = 0;
		float input_audio[num_frames * in_stream->getChannelCount()];

		// FIXME: [Florian] remove this ugly code from the FullDuplexStream sample
		if (m_callbackstodrain > 0) {
			int totalFramesRead = 0;
			do {
				auto result = in_stream->read(input_audio, num_frames, 0);
				// Ignore errors because input stream may not be started yet.
				if (!result) break;
				actualFramesRead = result.value();
				totalFramesRead += actualFramesRead;
			} while (actualFramesRead > 0);

			// Only counts if we actually got some data.
			if (totalFramesRead > 0) m_callbackstodrain--;
		}
		else if (m_callbackstocushion) {
			m_callbackstocushion--;
		}
		else {
			bool should_discard = m_callbackstodiscard;
			if (should_discard) m_callbackstodiscard--;

			auto result = in_stream->getAvailableFrames();
			if (!result) {
				WriteOutputSilence(stream, out_audio, num_frames);
				StopAudioAbnormal(in_stream);
				return oboe::DataCallbackResult::Stop;
			}

			int framesAvailable = result.value();
			if (framesAvailable >= num_frames) {
				auto resultRead = in_stream->read(input_audio, num_frames, 0);
				if (!resultRead) {
					WriteOutputSilence(stream, out_audio, num_frames);
					StopAudioAbnormal(stream);
					return oboe::DataCallbackResult::Stop;
				}

				if (!should_discard) {
					audio_read = true;
					ReadInputInterleaved(in_stream, input_audio, num_frames);
				}
			}
		}

		if (!audio_read) {
			REFLEX_FOREACH(buf, GetBuffers(false)) MemClear(buf, m_buffersize * sizeof(buf[0]));
		}
	}

	ProcessRt(scope, num_frames);

	if (m_stream[kDirectionOutput]) {
		if (!CheckStreamFormat(kDirectionOutput, stream, num_frames)) {
			WriteOutputSilence(stream, out_audio, num_frames);
			StopAudioAbnormal(stream);
			return oboe::DataCallbackResult::Stop;
		}

		WriteOutputInterleaved(stream, out_audio, num_frames);
	}

	return oboe::DataCallbackResult::Continue;
}

void AudioApp::onErrorAfterClose(oboe::AudioStream* stream, oboe::Result result) {
	DEV_LOG("AudioApp::onError", int(result));
	if (result == oboe::Result::ErrorDisconnected) {
		StopAudioAbnormal(stream);
	}
	return AudioStreamErrorCallback::onErrorAfterClose(stream, result);
}

AudioApp::Channel* AudioApp::GetAnyEnabledChannel(int direction) {
	//TODO REVIEW
	REFLEX_FOREACH(ch, m_availablechannels[direction]) {
		if (ch.enabled) return &ch;
	}
	return nullptr;
}

void AudioApp::BuildStream(const Channel& c, oboe::AudioStreamBuilder& builder) const {
	/// MEMO: for now, we support only float.
	builder.setFormat(oboe::AudioFormat::Float);
	builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
	builder.setDeviceId(c.device_id);
	builder.setSampleRate(int(m_samplerate));
	builder.setFramesPerCallback(int(m_buffersize));
	builder.setBufferCapacityInFrames(int(m_buffersize));
	builder.setChannelCount(c.sibling_count);
}

bool AudioApp::OpenTestStream(int direction, int deviceid, Float32 samplerate, UInt32 buffersize, std::shared_ptr<oboe::AudioStream>& out_stream) {
	oboe::AudioStreamBuilder builder;
	builder.setFormat(oboe::AudioFormat::Float);
	builder.setSampleRate(int(samplerate));
	builder.setDeviceId(deviceid);
	builder.setBufferCapacityInFrames(int(buffersize));
	builder.setDirection(direction == kDirectionOutput ? oboe::Direction::Output : oboe::Direction::Input);
	return builder.openStream(out_stream) == oboe::Result::OK && out_stream;
}

REFLEX_END

Reflex::WString Reflex::System::AudioPlugin::GetDefaultDevice(bool recording) {
	return Android::AudioApp::kDefaultDevice;
}

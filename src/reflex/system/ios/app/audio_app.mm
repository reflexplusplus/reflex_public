#include "audio_app.h"

#include "ReflexAudioEffectNode.mm"




REFLEX_NS(Reflex::System::iOS)

/**
 MEMO: we don't support setting the output device for now. It might or might not be possible. See the following:
 https://stackoverflow.com/questions/12264697/how-to-list-available-audio-output-route-on-ios
 https://stackoverflow.com/questions/15643516/list-available-output-audio-target-avaudiosession
 */
AudioApp::AudioApp()
	: m_audioSessionNotifications([NSMutableArray new])
	, m_session([AVAudioSession sharedInstance])
	, m_audiodevices({ kDefaultDevice })
{
	auto onRouteChange = [] (NSNotification* n) {
		if (auto app = Cast<AudioApp>(AudioAppBase::Get())) {
			Lock lock(*app);
			app->CollectCurrentDeviceInfo(true);
			globals->m_signals[kNotificationChangeDevices].Notify();
		}
	};

	auto onInterruption = [was_running = false] (NSNotification* n) mutable {
		if (auto app = Cast<AudioApp>(AudioApp::Get())) {
			NSNumber* interruptionType = n.userInfo[AVAudioSessionInterruptionTypeKey];

			switch (interruptionType.unsignedIntegerValue) {
				case AVAudioSessionInterruptionTypeBegan:
					was_running = app->m_engine.isRunning;
					app->OnPause();
					break;

				case AVAudioSessionInterruptionTypeEnded: {
					NSNumber* interruptionOption = n.userInfo[AVAudioSessionInterruptionOptionKey];
					if (interruptionOption.unsignedIntegerValue == AVAudioSessionInterruptionOptionShouldResume && was_running) {
						was_running = false;
						app->OnResume();
					}
					break;
				}
			}
		}
	};

	auto* __weak center = NSNotificationCenter.defaultCenter;
	// This one has newDeviceAvailable and oldDeviceUnavailable
	// https://developer.apple.com/documentation/avfaudio/responding-to-audio-route-changes
	// Ran from the main thread (checked)
	[m_audioSessionNotifications addObject:
	 [center addObserverForName:AVAudioSessionRouteChangeNotification
						 object:m_session queue:NSOperationQueue.mainQueue
					 usingBlock:onRouteChange]];
	// Interruptions have a began and ended state, so we should be able to resume afterwards:
	// https://developer.apple.com/documentation/avfaudio/handling-audio-interruptions
	[m_audioSessionNotifications addObject:
	 [center addObserverForName:AVAudioSessionInterruptionNotification
						 object:m_session queue:NSOperationQueue.mainQueue
					 usingBlock:onInterruption]];
	// https://developer.apple.com/documentation/avfaudio/avaudiosession/mediaserviceswereresetnotification
	[m_audioSessionNotifications addObject:
	 [center addObserverForName:AVAudioSessionMediaServicesWereResetNotification
						 object:m_session queue:NSOperationQueue.mainQueue
					 usingBlock:onRouteChange]];
}

AudioApp::~AudioApp() {
	DestroyEngine();
	for (id<NSObject> n in m_audioSessionNotifications) {
		[NSNotificationCenter.defaultCenter removeObserver:n];
	}
}

void AudioApp::DeactivateSession() {
	if (!m_session_active) return;
	NSError* error;

	if (![m_session setActive:false withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation error:&error]) {
		DEV_WARN("Could not deactivate Audio Session:", ToCStringView(error.description));
	}
	// OK to mark the session as stopped even when provoked by an error
	m_session_active = false;
}

void AudioApp::ActivateSession() {
	if (m_session_active) return;

	REFLEX_DISABLE_WARNINGS
	NSError* error;
	bool uses_inputs = GetSelectedChannelCount(kDirectionInput) > 0;
	auto mode = AVAudioSessionModeDefault; // MEMO: there are other modes available
	auto category = uses_inputs ? AVAudioSessionCategoryPlayAndRecord : AVAudioSessionCategoryPlayback;
	auto options = kAllowMixingWithOtherApps ? AVAudioSessionCategoryOptionMixWithOthers : 0u;
	if (uses_inputs) {
		options |= (AVAudioSessionCategoryOptionDefaultToSpeaker |
					AVAudioSessionCategoryOptionAllowBluetooth |
					AVAudioSessionCategoryOptionAllowBluetoothA2DP |
					AVAudioSessionCategoryOptionAllowAirPlay);
	}
	REFLEX_ENABLE_WARNINGS

	if (![m_session setCategory:category mode:mode options:options error:&error]) {
		DEV_WARN("Could not activate Audio Session:", ToCStringView(error.description));
		return;
	}

	if (@available(iOS 17.0, *)) {
		if (![m_session setPrefersInterruptionOnRouteDisconnect:NO error:&error]) {
			DEV_WARN("Failed to set route preferences", ToCStringView(error.description));
		}
	}

	if (@available(iOS 14.5, *)) {
		if (![m_session setPrefersNoInterruptionsFromSystemAlerts:YES error:&error]) {
			DEV_WARN("Failed to set route preferences", ToCStringView(error.description));
		}
	}

	// MEMO: Note that activating an audio session is a synchronous (blocking) operation. Apps may activate a AVAudioSessionCategoryPlayback session when another app is hosting a call (to start a SharePlay activity for example). However, they are not permitted to capture the microphone of the active call, so attempts to activate a session with category AVAudioSessionCategoryRecord or AVAudioSessionCategoryPlayAndRecord will fail with error AVAudioSessionErrorCodeInsufficientPriority.
	if (![m_session setActive:true withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation error:&error]) {
		DEV_WARN("Could not activate Audio Session:", ToCStringView(error.description));
		return;
	}
	m_session_active = true;
}

void AudioApp::CreateEngine() {
	if (!m_engine) {
		m_engine = [AVAudioEngine new];

		auto* __weak center = NSNotificationCenter.defaultCenter;
		m_engineConfigurationChangeNotification = [center addObserverForName:AVAudioEngineConfigurationChangeNotification
																	  object:m_engine queue:NSOperationQueue.mainQueue
																  usingBlock:[] (NSNotification* note)
		{
			if (auto app = AudioApp::Get()) {
				Cast<AudioApp>(app)->OnAudioFormatPotentiallyChanged();
			}
		}];
	}
}

void AudioApp::StartEngine() {
	REFLEX_ASSERT(m_session_active)
	REFLEX_ASSERT(m_engine)
	REFLEX_ASSERT(!m_engine.isRunning)

	auto uses_inputs = GetSelectedChannelCount(kDirectionInput) > 0;
	auto sr = GetCurrentSampleRate();
	auto bs = GetCurrentBufferSize();

	DEV_LOG("AudioApp::Starting audio", m_currentdevice, sr, bs);
	UpdateBuffers(bs);
	PrepareProcessing(bs, sr);

	if (!uses_inputs) {
		// Setup for just output (simpler)
		m_outputformat = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32 sampleRate:sr channels:kNumOutputChannels interleaved:false];
		m_processingnode = [[AVAudioSourceNode alloc] initWithFormat:m_outputformat renderBlock:[](BOOL *isSilence, const AudioTimeStamp *timestamp, AVAudioFrameCount frameCount, AudioBufferList *outputData) -> OSStatus {
			// Session can be temporarily deactivated
			auto app = Cast<AudioApp>(AudioApp::Get());
			if (!app || *isSilence) {
				WriteOutputSilence(outputData, frameCount);
				return noErr;
			}

			app->ProcessAudioInputOutput(outputData, frameCount, false);
			return noErr;
		}];

		if (m_processingnode) {
			[m_engine attachNode:m_processingnode];
			[m_engine connect:m_processingnode to:m_engine.outputNode format:m_outputformat];
		}
		else {
			m_outputformat = nil;
			DEV_WARN("Could not create output node");
		}
	}
	else {
		// Setup for low latency input processing
		m_outputformat = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32 sampleRate:sr channels:kNumOutputChannels interleaved:false];
		m_inputformat = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32 sampleRate:sr channels:kNumInputChannels interleaved:false];

		// Output format (and channel count) is used as a reference in the callback. We can have less but no more input channels. We could use a AVAudioMixerNode to mix them down.
		REFLEX_ASSERT(kNumInputChannels <= kNumOutputChannels)

		m_processingnode = [[ReflexAudioEffectNode alloc] initWithFormat:m_outputformat renderBlock:[](AudioUnitRenderActionFlags* actionFlags, const AudioTimeStamp* timestamp, AUAudioFrameCount frameCount, NSInteger outputBusNumber, AudioBufferList* outputData, const AURenderEvent* realtimeEventListHead, AURenderPullInputBlock __unsafe_unretained pullInputBlock) -> OSStatus {

			auto app = Cast<AudioApp>(AudioApp::Get());

			if (!app) {
				WriteOutputSilence(outputData, frameCount);
				return noErr;
			}

			auto inputStatus = pullInputBlock(actionFlags, timestamp, frameCount, 0, outputData);
			if (inputStatus != noErr) {
				NSLog(@"Unable to pull audio: %@", [NSError errorWithDomain:NSOSStatusErrorDomain code:inputStatus userInfo:nil]);
				return inputStatus;
			}

			app->ProcessAudioInputOutput(outputData, frameCount, true);
			return noErr;
		}];

		[m_engine attachNode:m_processingnode];
		[m_engine connect:m_engine.inputNode to:m_processingnode format:m_outputformat];
		[m_engine connect:m_processingnode to:m_engine.outputNode format:m_outputformat];
	}

	NSError* error;
	if (![m_engine startAndReturnError:&error]) {
		DEV_WARN("Could not start Audio:", ToCStringView(error.description));
	}
}

void AudioApp::StopEngine() {
	[m_engine stop];
	if (m_processingnode)  [m_engine detachNode:m_processingnode];
	m_processingnode = nil;
	m_outputformat = m_inputformat = nil;
}

void AudioApp::DestroyEngine() {
	REFLEX_ASSERT(!m_engine.isRunning)
	if (m_engineConfigurationChangeNotification) {
		[NSNotificationCenter.defaultCenter removeObserver:m_engineConfigurationChangeNotification];
		m_engineConfigurationChangeNotification = nil;
	}
	m_engine = nil;
	DeactivateSession();
}

void AudioApp::UpdateBuffers(UInt32 buffersize) {
	REFLEX_ASSERT(buffersize > 0)

	ClearBuffers();
	m_silence_buffer.SetSize(buffersize);
	m_silence_buffer.Wipe();

	REFLEX_LOOP(direction, kNumDirection) {
		auto& channels = m_channels[direction];
		auto& buffers = m_channelbuffers[direction];
		UInt32 nchannel = channels.GetSize();

		buffers.SetSize(nchannel);

		REFLEX_LOOP(idx, nchannel) {
			auto& channel = channels[idx];
			auto& buffer = buffers[idx];

			if (channel.enabled) {
				buffer.SetSize(buffersize);
				buffer.Wipe();

				channel.buffer = buffer.GetData();
				AddBuffer(direction, channel.buffer);
			}
			else {
				buffer.Clear();
				channel.buffer = m_silence_buffer.GetData();
			}
		}
	}
}

void AudioApp::CollectCurrentDeviceInfo(bool preserveChannelStates) {
	Map<WString, bool> channelsState[kNumDirection];
	m_availablebuffersizes.Clear();
	m_availablesamplerates.Clear();
	REFLEX_LOOP(direction, kNumDirection) {
		if (preserveChannelStates) {
			REFLEX_FOREACH(ch, m_channels[direction]) {
				channelsState[direction].Set(ch.name, ch.enabled);
			}
		}
		m_channels[direction].Clear();
		m_channelbuffers[direction].Clear();
	}

	if (!m_currentdevice) return;

	REFLEX_ASSERT(m_session_active && !m_engine.isRunning)

	// Channels
	REFLEX_LOOP(direction, kNumDirection) {
		auto& channels = m_channels[direction];
		auto channelName = direction ? L"Output Ch." : L"Input Ch.";
		UInt num_channels = direction ? kNumOutputChannels : kNumInputChannels;

		channels.Clear();
		REFLEX_LOOP(chn, num_channels) {
			WString name = Join(channelName, Reflex::ToWString(chn + 1));
			channels.Push({
				.name = name,
				.enabled = channelsState[direction].Acquire(name, false),
				.buffer = nullptr
			});
		}
	}

	// Buffer sizes
	auto current_sr = m_session.sampleRate;
	auto current_dur = m_session.IOBufferDuration;
	auto current_bs = BufferDurationToSize(current_sr, current_dur);
	NSError* error;

	REFLEX_FOREACH(bs, kStandardBufferSizes) {
		auto duration = BufferSizeToDuration(current_sr, bs);
		if ([m_session setPreferredIOBufferDuration:duration error:&error]) {
			auto real_bs = BufferDurationToSize(current_sr, m_session.IOBufferDuration);
			if (!Search(m_availablebuffersizes, real_bs)) m_availablebuffersizes.Push(real_bs);
		}
	}

	// Sample rates
	REFLEX_FOREACH(sr, kStandardSampleRates) {
		if ([m_session setPreferredSampleRate:sr error:&error]
			&& IsAlmostEqual(sr, Float32(m_session.sampleRate)))
		{
			m_availablesamplerates.Push(sr);
		}
	}

	if (m_availablebuffersizes.Empty()) m_availablebuffersizes.Push(current_bs);
	if (m_availablesamplerates.Empty()) m_availablesamplerates.Push(current_sr);

	[m_session setPreferredSampleRate:current_sr error:&error];

	if (m_lastbuffersize == 0 || !preserveChannelStates) {
		m_lastbuffersize = current_bs;
	}
	m_lastbuffersize = FindClosest(m_availablebuffersizes, m_lastbuffersize);
	[m_session setPreferredIOBufferDuration:BufferSizeToDuration(current_sr, m_lastbuffersize) error:&error];
}

UInt AudioApp::GetHardwareChannelCount(int direction) const {
	if (!m_session_active) return 0;

	// Also has m_session.outputNumberOfChannels and m_session.inputNumberOfChannels, values sometimes inconsistent with below
	return direction == kDirectionOutput
		? UInt([m_engine.outputNode outputFormatForBus:kFirstAudioBus].channelCount)
		: UInt([m_engine.inputNode inputFormatForBus:kFirstAudioBus].channelCount); // Triggers a "microphone permission" dialog
}

UInt AudioApp::GetSelectedChannelCount(int direction) const {
	UInt count = 0;
	REFLEX_FOREACH(c, m_channels[direction]) {
		count += c.enabled ? 1 : 0;
	}
	return count;
}

void AudioApp::ProcessAudioInputOutput(AudioBufferList* hwbuffers, AUAudioFrameCount num_frames, bool has_input) {
	ProcessingScopeRt scope(this);
	if (scope.IsLocked()) {
		WriteOutputSilence(hwbuffers, num_frames);
		return;
	}

#if REFLEX_DEBUG
	static DebouncedAction<2000, false> notifier;
	static unsigned total_calls = 0, divider = 0;
	unsigned calls_this_time = 0;
	notifier.Perform([&] {
		if (total_calls > divider) {
			DEV_LOG("TEMP: calls per ProcessAudioForOutput", Float32(total_calls) / divider, "typical buffer", num_frames, m_silence_buffer.GetSize(), GetCurrentBufferSize());
		}
		total_calls = divider = 0;
	});
#endif

	UInt32 num_channels = hwbuffers->mNumberBuffers;
	if (m_channels[kDirectionOutput].GetSize() != num_channels // channel count didn't change
		|| (num_channels > 0 && hwbuffers->mBuffers[0].mDataByteSize != sizeof(float) * num_frames)) // non-interleaved
	{
		dispatch_async(dispatch_get_main_queue(), ^{
			DEV_WARN("Stream format invalid, stopping");
			StopEngine();
		});
		return;
	}

	// All buffers have the same size (see UpdateBuffers)
	auto buffersize = m_silence_buffer.GetSize();
	UInt processed = 0;

	while (processed < num_frames) {
		auto this_time = Min(num_frames - processed, buffersize);

		if (has_input) ReadInputNonInterleaved(m_channels[kDirectionInput], hwbuffers, processed, this_time);
		ProcessRt(scope, this_time);
		WriteOutputNonInterleaved(m_channels[kDirectionOutput], hwbuffers, processed, this_time);

		processed += this_time;

#if REFLEX_DEBUG
		calls_this_time += 1;
#endif
	}

	// TODO: Florian -- MIDI Out

	//MIDISend

#if REFLEX_DEBUG
	total_calls += calls_this_time;
	divider += 1;
#endif
}

void AudioApp::WriteOutputSilence(AudioBufferList* output, UInt buffersize) {
	REFLEX_LOOP(idx, output->mNumberBuffers) {
		auto& audiobuffer = output->mBuffers[idx];
		MemClear(audiobuffer.mData, buffersize * audiobuffer.mNumberChannels * sizeof(Float32));
	}
}

void AudioApp::WriteOutputNonInterleaved(const Array<Channel>& source, AudioBufferList* dest, UInt destoffset, UInt frame_count) {
	// Should be checked before calling!
	REFLEX_ASSERT(source.GetSize() == dest->mNumberBuffers)

	REFLEX_LOOP(chn_idx, dest->mNumberBuffers) {
		AudioBuffer& destbuf = dest->mBuffers[chn_idx];
		REFLEX_ASSERT(sizeof(Float32) * (destoffset + frame_count) <= destbuf.mDataByteSize);
		MemCopy(source[chn_idx].buffer, Reinterpret<Float32>(destbuf.mData) + destoffset, frame_count * sizeof(Float32));
	}
}

void AudioApp::ReadInputNonInterleaved(const Array<Channel>& dest, AudioBufferList* source, UInt sourceoffset, UInt frame_count) {
	// With CoreAudio, the input buffer is the same as the output buffer (in-place modification), so we use the number of outputs to set up the buffer, and
	// let CoreAudio map the inputs to the output buffer. If there are less input channels than output channels, the data will be zero for these channels.
	auto input_channels = Min(dest.GetSize(), source->mNumberBuffers);

	REFLEX_LOOP(chn_idx, input_channels) {
		if (!dest[chn_idx].enabled) continue;

		AudioBuffer& sourcebuf = source->mBuffers[chn_idx];
		REFLEX_ASSERT(sizeof(Float32) * (sourceoffset + frame_count) <= sourcebuf.mDataByteSize);
		MemCopy(Reinterpret<Float32>(sourcebuf.mData) + sourceoffset, dest[chn_idx].buffer, frame_count * sizeof(Float32));
	}
}

void AudioApp::OnAudioFormatPotentiallyChanged() {
	if (!m_engine.isRunning) return;

	auto newSampleRate = GetCurrentSampleRate();
	if ((m_outputformat && FormatRequiresRestart(m_outputformat, newSampleRate, kNumOutputChannels)) ||
		(m_inputformat && FormatRequiresRestart(m_inputformat, newSampleRate, kNumInputChannels)))
	{
		Lock lock(*this);
		CollectCurrentDeviceInfo(true);
	}
}

// MARK: inherited
Array<WString> AudioApp::GetAvailableAudioDevices() const {
	return m_audiodevices;
}

bool AudioApp::OnSelectAudioDevice(const WString::View& value) {
	if (value == kDefaultDevice) {
		DeactivateSession();
		DestroyEngine();

		m_channels[kDirectionInput].Clear();
		m_channels[kDirectionOutput].Clear();

		ActivateSession();
		CreateEngine();
		m_currentdevice = value;

		CollectCurrentDeviceInfo(false);
		return true;
	}

	return false;
}

WString AudioApp::GetCurrentAudioDevice() const {
	return m_currentdevice;
}

UInt32 AudioApp::GetCurrentBufferSize() const {
	if (!m_session_active) return kDefaultBufferSize;

	return BufferDurationToSize(m_session.sampleRate, m_session.IOBufferDuration);
}

Float AudioApp::GetCurrentSampleRate() const {
	if (!m_session_active) return kDefaultSampleRate;

	return m_session.sampleRate;
}

void AudioApp::OnRequestBufferSize(UInt32 desiredBufferSize) {
	if (!m_session_active) return;

	NSError* error;

	auto duration = BufferSizeToDuration(m_session.sampleRate, desiredBufferSize);
	if (![m_session setPreferredIOBufferDuration:duration error:&error] || error) {
		DEV_WARN("Could not set buffer size:", ToCStringView(error.description));
	}
	else {
		m_lastbuffersize = desiredBufferSize;
	}
}

void AudioApp::OnRequestSampleRate(Float desiredSampleRate) {
	if (!m_session_active) return;

	NSError* error;
	auto bs = GetCurrentBufferSize();
	if (![m_session setPreferredSampleRate:desiredSampleRate error:&error] || error) {
		DEV_WARN("Could not set sample rate:", ToCStringView(error.description));
	}

	// Buffer size is dependent on the sample rate so we need to reapply it
	OnRequestBufferSize(bs);
}

Array<UInt32> AudioApp::GetAvailableBufferSizes() const {
	return m_availablebuffersizes;
}

Array<Float32> AudioApp::GetAvailableSampleRates() const {
	return m_availablesamplerates;
}

Array<Pair<WString,bool>> AudioApp::GetAudioChannels(bool output) const {
	Array<Pair<WString,bool>> rtn;
	auto& channels = m_channels[output];

	rtn.Allocate(channels.GetSize());
	for (auto & i : channels) rtn.Push({ i.name, i.enabled });

	return rtn;
}

void AudioApp::OnEnableAudioChannel(bool output, UInt32 idx, bool enable) {
	m_channels[output][idx].enabled = enable;
}

void AudioApp::OnPause() {
	StopEngine();
}

void AudioApp::OnResume() {
	if (m_session_active) {
		StartEngine();
	}
}

REFLEX_END

Reflex::WString Reflex::System::AudioPlugin::GetDefaultDevice(bool recording) {
	return iOS::AudioApp::kDefaultDevice;
}

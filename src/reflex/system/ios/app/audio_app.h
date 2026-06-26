// MEMO: looks like it could do: https://www.momentslog.com/development/ios/building-a-music-streaming-app-with-core-audio-and-audiokit-in-objective-c
#pragma once

#include "../sdk.h"
#import <AVFoundation/AVFoundation.h>

#include "../../common/instance/audioapp.h"
#include "../../common/instance/eventqueue.h"




//
//declarations

REFLEX_NS(Reflex::System::iOS)

struct AudioApp : public Common::MobileAudioAppBase {
	static constexpr WString::View kDefaultDevice = L"iOS audio";
	static constexpr UInt kNumInputChannels = 2;
	static constexpr UInt kNumOutputChannels = 2;

	//lifetime
	AudioApp();
	~AudioApp() override;



private:

	static constexpr bool kAllowMixingWithOtherApps = true;
	static constexpr AVAudioNodeBus kFirstAudioBus = 0;
	// We have two devices on iOS to avoid requesting the microphone permission unless necessary

	enum {
		kDirectionInput = 0,
		kDirectionOutput = 1,
		kNumDirection,
	};

	struct Channel {
		WString name;
		bool enabled;
		Float32* buffer;
	};

	// Device & channel management
	Array<WString> GetAvailableAudioDevices() const override;
	bool OnSelectAudioDevice(const WString::View & value) override;
	WString GetCurrentAudioDevice() const override;
	UInt32 GetCurrentBufferSize() const override;
	Float32 GetCurrentSampleRate() const override;

	void OnRequestBufferSize(UInt32 value) override;
	void OnRequestSampleRate(Float32 value) override;

	Array<UInt32> GetAvailableBufferSizes() const override;
	Array<Float32> GetAvailableSampleRates() const override;

	Array<Pair<WString,bool>> GetAudioChannels(bool output) const override;
	void OnEnableAudioChannel(bool output, UInt32 idx, bool enable) override;

	// TODO: Florian -- MIDI
	Array<Pair<WString,bool>> GetMidiPorts(bool output) const override { return {}; }
	void OnEnableMidiPort(bool output, UInt idx, bool enable) override {}

	void OnPause() override;
	void OnResume() override;


	// 1) Session then 2) Engine
	void ActivateSession();
	void DeactivateSession();

	void CreateEngine();
	void StartEngine(); // Will call user methods, such as PrepareProcessing
	void StopEngine();
	void DestroyEngine();
	void UpdateBuffers(UInt32 buffersize);
	void CollectCurrentDeviceInfo(bool preserveChannelStates);

	double BufferSizeToDuration(Float32 sr, UInt32 bs) const {
		/**
		 Memo: the 0.1 is to avoid rounding down that iOS 18 may make (setting a duration that's 256/sr, might have the OS make a 128-sample buffer, while setting 257/sr would give a 256-sample buffer).
		 Also, note that it doesn't work on the iOS simulator as of iOS 18.2 on macOS 15.1.1. The duration is updated but the callback (ProcessAudioForOutput) will always be called with ~512 frames.
		 https://stackoverflow.com/questions/35468522/avaudiosession-buffer-duration-on-iphone6-simulator
		 */
		return (bs + 0.1) / sr;
	}
	UInt32 BufferDurationToSize(Float32 sr, double duration) const {
		return UInt32(RoundNearest(duration * sr));
	}

	UInt GetHardwareChannelCount(int direction) const;
	UInt GetSelectedChannelCount(int direction) const;

	// Internal audio processing
	void ProcessAudioInputOutput(AudioBufferList* hwbuffers, AUAudioFrameCount num_frames, bool has_input);

	static void WriteOutputSilence(AudioBufferList* output, UInt frame_count);
	static void WriteOutputNonInterleaved(const Array<Channel>& source, AudioBufferList* dest, UInt destoffset, UInt frame_count);
	static void ReadInputNonInterleaved(const Array<Channel>& dest, AudioBufferList* source, UInt sourceoffset, UInt frame_count);

	void OnAudioFormatPotentiallyChanged();
	bool FormatRequiresRestart(AVAudioFormat* previous, double samplerate, AVAudioChannelCount channelcount) {
		return !IsAlmostEqual(previous.sampleRate, samplerate) || previous.channelCount != channelcount;
	}


	// Members
	Array<WString> m_audiodevices;
	WString m_currentdevice;
	Array<Channel> m_channels[kNumDirection];

	Array<Float32> m_silence_buffer;
	Array<Array<Float32>> m_channelbuffers[kNumDirection];
	Array<Float32> m_availablesamplerates;
	Array<UInt32> m_availablebuffersizes;
	UInt32 m_lastbuffersize = 0;

	std::atomic<bool> m_session_active = false;
	AVAudioSession* __weak m_session = nil;
	AVAudioEngine* m_engine = nil;
	bool m_started = false;
	AVAudioNode* m_processingnode = nil;
	AVAudioFormat* m_inputformat = nil;
	AVAudioFormat* m_outputformat = nil;
	NSMutableArray<id<NSObject>>* m_audioSessionNotifications;
	id<NSObject> m_engineConfigurationChangeNotification;
};

REFLEX_END

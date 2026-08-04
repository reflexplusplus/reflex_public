#include "sdk.h"
#include "../common/instance/audioapp.h"




//
//internal

REFLEX_BEGIN_INTERNAL(Reflex::System::WebASM)

struct AudioAppImpl : public Common::AudioAppBase
{
	static constexpr WString::View kPlaybackDevice = L"0 In / 2 Out";

	AudioAppImpl()
		: m_system(Start(0))
		, m_webAudioContext(0)
		, m_sr(kDefaultSampleRate)
	{
	}

	void OnPause() override;

	void OnResume() override;

	Array < Pair <WString,bool> > GetMidiPorts(bool output) const override;

	void OnEnableMidiPort(bool output, UInt idx, bool enable) override;


	bool OnSelectAudioDevice(const WString::View & value) override;

	WString GetCurrentAudioDevice() const override { return {}; }

	Array <WString> GetAvailableAudioDevices() const override;


	void OnRequestBufferSize(UInt32 value) override;

	UInt32 GetCurrentBufferSize() const override { return 1024; }

	Array <UInt32> GetAvailableBufferSizes() const override { return ToView(kStandardBufferSizes); }


	void OnRequestSampleRate(Float32 value) override;

	Float32 GetCurrentSampleRate() const override { return m_sr; }

	Array <Float32> GetAvailableSampleRates() const override { return ToView(kStandardSampleRates); }


	Array < Pair <WString,bool> > GetAudioChannels(bool output) const override;

	void OnEnableAudioChannel(bool output, UInt32 idx, bool enable) override;


	static bool OnProcess(int numInputs, const AudioSampleFrame *inputs, int numOutputs, AudioSampleFrame *outputs, int numParams, const AudioParamFrame * params, void * userData)
	{
		auto & self = *Cast<AudioAppImpl>(userData);

		ProcessingScopeRt scope(&self);

		if (scope.IsLocked()) return true;

		self.ProcessRt(scope, 128);

		auto buffers = self.m_buffers;

		REFLEX_LOOP_PTR(outputs, pbus, numOutputs)
		{
			if (pbus->numberOfChannels == 2 && pbus->samplesPerChannel == 128)
			{
				auto bytes = 128 * sizeof(Float32);

				MemCopy(buffers[0], pbus->data, bytes);
				MemCopy(buffers[1], pbus->data + 128, bytes);
			}
		}

		return true; // Keep the graph output going
	}

	static void OnAudioWorkletProcessorCreated(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void *userData)
	{
		if (!success) return; // Check browser console in a debug build for detailed errors

		int outputChannelCounts[1] = { 2 };
		
		EmscriptenAudioWorkletNodeCreateOptions options = 
		{
			.numberOfInputs = 0,
			.numberOfOutputs = 1,
			.outputChannelCounts = outputChannelCounts
		};

		auto self = Cast<AudioAppImpl>(Common::AudioAppBase::Get());

		EMSCRIPTEN_AUDIO_WORKLET_NODE_T wasmAudioWorklet = emscripten_create_wasm_audio_worklet_node(audioContext, "reflex", &options, &AudioAppImpl::OnProcess, self);

		emscripten_audio_node_connect(wasmAudioWorklet, audioContext, 0, 0);

		emscripten_set_click_callback("#start", self, 0, &AudioAppImpl::OnCanvasClick);
	}

	static void OnAudioThreadInitialized(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void *userData)
	{
		if (!success) return; // Check browser console in a debug build for detailed errors
	 
		WebAudioWorkletProcessorCreateOptions opts = { .name = "reflex", };
	  
		emscripten_create_wasm_audio_worklet_processor_async(audioContext, &opts, &AudioAppImpl::OnAudioWorkletProcessorCreated, 0);
	}

	static bool OnCanvasClick(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData)
	{
		if (auto audioContext = Cast<AudioAppImpl>(userData)->m_webAudioContext)
		{
			if (emscripten_audio_context_state(audioContext) != AUDIO_CONTEXT_STATE_RUNNING) 
			{
				emscripten_resume_audio_context_sync(audioContext);
			}
		}

		return false;
	}


	Reference <Object> m_system;

	EMSCRIPTEN_WEBAUDIO_T m_webAudioContext;

	Float32 m_sr;

	Float32 m_buffers[2][128];
	
	static inline uint8_t st_stack_rt[1024 * 1024];		//no idea if this is enough...  (example is 4096 bytes)
};

void AudioAppImpl::OnResume()
{
	EmscriptenWebAudioCreateAttributes attributes = { .latencyHint = "interactive", .sampleRate = 44100 };

	m_sr = 44100.0f;

	m_webAudioContext = emscripten_create_audio_context(&attributes);

	emscripten_start_wasm_audio_worklet_thread_async(m_webAudioContext, st_stack_rt, sizeof(st_stack_rt), &AudioAppImpl::OnAudioThreadInitialized, 0);	

	ClearBuffers();

	AddBuffer(true, m_buffers[0]);

	AddBuffer(true, m_buffers[1]);

	if (PrepareProcessing(128, m_sr))
	{
	}
}

void AudioAppImpl::OnPause()
{
	emscripten_destroy_audio_context(m_webAudioContext);
	
	m_webAudioContext = 0;
}

Array < Pair <WString,bool> > AudioAppImpl::GetMidiPorts(bool output) const
{
	Array < Pair <WString,bool> > rtn;

	return rtn;
}

void AudioAppImpl::OnEnableMidiPort(bool output, UInt idx, bool enable)
{
}

Array <WString> AudioAppImpl::GetAvailableAudioDevices() const
{
	return { kPlaybackDevice } ;
}

bool AudioAppImpl::OnSelectAudioDevice(const WString::View & value)
{
	return value == kPlaybackDevice;
}

void AudioAppImpl::OnRequestBufferSize(UInt32 value)
{
}

void AudioAppImpl::OnRequestSampleRate(Float32 value)
{
}

Array < Pair <WString, bool> > AudioAppImpl::GetAudioChannels(bool output) const
{
	Array < Pair <WString, bool> > rtn;

	if (output)
	{
		rtn = { {L"Left", true}, {L"Right", true} };
	}

	return rtn;
}

void AudioAppImpl::OnEnableAudioChannel(bool output, UInt32 idx, bool enable)
{
}

REFLEX_END_INTERNAL

const Reflex::System::EnvironmentType Reflex::System::kEnvironmentType = Reflex::System::kEnvironmentTypeMobileApp;

void Reflex::System::App::Quit()
{
}

Reflex::WString Reflex::System::AudioPlugin::GetDefaultDevice(bool recording)
{
	return WebASM::AudioAppImpl::kPlaybackDevice;
}

int main()
{
	auto core = Reflex::Start();	//start core first so g_default_allocator is ready

	auto instance = REFLEX_CREATE(Reflex::System::WebASM::AudioAppImpl);

	instance->Initialise("");

	return 0;
}

REFLEX_WASM_EXPORT void reflexOnUnload()
{
	//not so much point to do this, but...

	REFLEX_USE(Reflex::System);

	Cast<WebASM::AudioAppImpl>(WebASM::AudioAppImpl::Get())->Deinitialise();
}

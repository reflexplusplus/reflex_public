#pragma once

#include "standalone.h"
#include "eventqueue.h"




//
//declarations

REFLEX_NS(Reflex::System::Common)

class AudioAppBase : public Standalone <AudioPlugin>
{
public:

	//types

	struct ProcessingScopeRt;


	//globals

	static constexpr Float32 kDefaultSampleRate = 44100.0f;

	static constexpr UInt32 kDefaultBufferSize = 512;

	static constexpr Float32 kStandardSampleRates[] = { 11025.0f, 22050.0f, 44100.0f, 48000.0f, 88200.0f, 96000.0f, 192000.0f };

#if REFLEX_DEBUG
	static constexpr UInt32 kStandardBufferSizes[] = { 32, 64, 128, 256, 384, 512, 522, 1024, 2048, 4096 };
#else
	static constexpr UInt32 kStandardBufferSizes[] = { 32, 64, 128, 256, 384, 512, 1024, 2048 };
#endif



protected:

	AudioAppBase()
		: m_process_rt([](Callbacks&, UInt) {})
		, m_audio_started(false)
		, m_updated(false)
		, m_isprocessing_rt(false)
		, m_lockcount_fg(1)
	{
	}

	~AudioAppBase()
	{
		REFLEX_ASSERT(And(m_lockcount_fg, !REFLEX_ATOMIC_READ(m_isprocessing_rt)));
	}

	void ClearBuffers();

	void AddBuffer(bool output, Float * channel);

	const Array <Float*> & GetBuffers(bool output) { return m_audiochannels[output]; }

	bool PrepareProcessing(UInt32 buffersize, Float32 samplerate);

	template <class EVENT> REFLEX_INLINE void QueueEvent(AudioPlugin::Event::Type type, UInt16 position, UInt16 idx, EVENT value)
	{
		m_eventsinqueue.Append<EVENT>(type, position, idx, value);
	}

	ArrayView <AudioPlugin::Event> ProcessRt(const ProcessingScopeRt & scope, UInt samples);



	//callbacks (only called by lock)

	virtual void ReportProcessingDelay(UInt32 delay) final {}

	virtual void ReportStateChange(UInt8 change_flags) final {}

	virtual void BeginAutomation(UInt32 idx) final {}

	virtual void Automate(UInt32 idx, Float32 value) final { m_callbacks->OnSetParameterValue(idx, value); }

	virtual void EndAutomation(UInt32 idx) final {}



private:

	friend class AudioPlugin::Lock;

	friend struct ProcessingScopeRt;



	virtual void OnPause() = 0;

	virtual void OnResume() = 0;

	virtual void OnEnableMidiPort(bool output, UInt idx, bool enable) = 0;

	virtual bool OnSelectAudioDevice(const WString::View & value) = 0;

	virtual void OnEnableAudioChannel(bool output, UInt32 idx, bool enable) = 0;

	virtual void OnRequestBufferSize(UInt32 value) = 0;

	virtual void OnRequestSampleRate(Float32 value) = 0;


	virtual void OnStartAudio(Object & client) override final;

	virtual void OnStopAudio() override final;

	void Pause();

	void Resume();



	TRef <Callbacks> m_callbacks;

	EventInQueue m_eventsinqueue;

	AudioPlugin::EventBuffer m_eventsin, m_eventsout;

	Array <Float*> m_audiochannels[2];

	Pair <UInt32,Float32> m_config_z;

	FunctionPointer <void(Callbacks&, UInt)> m_process_rt;

	bool m_updated;

	bool m_audio_started;

	protected: AtomicUInt8 m_isprocessing_rt;	//rt writes, fg reads

	AtomicUInt32 m_lockcount_fg;	//fg writes, rt reads
};

struct AudioAppBase::ProcessingScopeRt
{
	ProcessingScopeRt(AudioAppBase * instance)
		: instance(*instance)
		, m_called_islocked(false)
	{
		REFLEX_ATOMIC_WRITE(instance->m_isprocessing_rt, true);
	}

	~ProcessingScopeRt()
	{
		REFLEX_ATOMIC_WRITE(instance.m_isprocessing_rt, false);

		REFLEX_ASSERT(m_called_islocked);
	}

	bool IsLocked() const
	{
		m_called_islocked = true;

		return REFLEX_ATOMIC_READ(instance.m_lockcount_fg);
	}

	AudioAppBase & instance;



private:

	friend class AudioAppBase;

	mutable bool m_called_islocked;
};

REFLEX_INLINE ArrayView <AudioPlugin::Event> AudioAppBase::ProcessRt(const ProcessingScopeRt & scope, UInt samples)
{
	REFLEX_ASSERT(scope.m_called_islocked);

	m_eventsin = m_eventsinqueue.Flush(false, 2.0f, 0.0);

	m_eventsout.events = {};

	m_process_rt(m_callbacks, samples);

	return m_eventsout.events;
}

typedef AudioAppBase DesktopAudioAppBase;

typedef AudioAppBase MobileAudioAppBase;

REFLEX_END
